#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt "\n"
#include <linux/kernel.h>

#include <linux/net.h>
#include <linux/socket.h>
#include <net/tcp.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <luadata.h>

#include "ulp.h"
#include "syscalls.h"
#include "pretty.h"

int ulp_recvmsg(struct sock *sk, struct msghdr *msg, size_t len,
      int nonblock, int flags, int *addr_len)
{
   lua_State *L = sk_ulp_data(sk)->L;
   struct context *ctx = sk_ulp_ctx(sk);
   int perr = 0;
   int data = LUA_NOREF;
   struct sk_buff *skb;
   struct tcphdr *hdr;

   lock_sock(sk);

   if (skb_queue_empty(&sk->sk_receive_queue) || !ctx->entry[0])
      goto out;

   if (lua_getglobal(L, ctx->entry) != LUA_TFUNCTION)
      goto out;

   skb = skb_peek_tail(&sk->sk_receive_queue);
   hdr = tcp_hdr(skb);
   if (unlikely(!hdr->psh))
      goto out;
   data = ldata_newref(L, skb->data, skb->len);
   perr = lua_pcall(L, 1, 1, 0);
   ldata_unref(L, data);
   if (unlikely(perr)) {
      pp_pcall(perr, lua_tostring(L, -1));
      goto out;
   }

   if (lua_toboolean(L, -1) == false)
      goto bad;

out:
   release_sock(sk);
   return sys->recvmsg(sk, msg, len, nonblock, flags, addr_len);

bad:
   release_sock(sk);
   return -ECONNREFUSED;
}

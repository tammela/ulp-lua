#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt "\n"
#include <linux/kernel.h>

#include <linux/net.h>
#include <linux/socket.h>
#include <net/tcp.h>
#include <net/busy_poll.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <luadata.h>

#include "ulp.h"
#include "syscalls.h"
#include "pretty.h"

int __doprocess(lua_State *L)
{
   struct sk_buff *skb = lua_touserdata(L, 1);
   struct context *ctx = luaU_getenv(L, struct context);
   int perr = 0;
   int data = LUA_NOREF;

   if (lua_getglobal(L, ctx->entry) != LUA_TFUNCTION)
      return luaL_error(L, "couldn't find context function: %s\n", ctx->entry);

   data = ldata_newref(L, skb->data, skb->len);
   perr = lua_pcall(L, 1, 1, 0);
   ldata_unref(L, data);
   if (perr)
      return lua_error(L);

   return 1;
}

int ulp_recvmsg(struct sock *sk, struct msghdr *msg, size_t len,
      int nonblock, int flags, int *addr_len)
{
   struct tcp_sock *tp = tcp_sk(sk);
   lua_State *L = sk_conn_ulp_data(sk)->L;
   struct context *ctx = sk_ulp_ctx(sk);
   struct sk_buff *skb;
   struct tcphdr *hdr;
   int perr;

   if (unlikely(sk->sk_state != TCP_ESTABLISHED))
      return -ENOTCONN;

   /* busy loop, if we can, while we are waiting for a data rx event */
   if (sk_can_busy_loop(sk) && skb_queue_empty(&sk->sk_receive_queue))
      sk_busy_loop(sk, nonblock);

   lock_sock(sk);

   /* fastpath to tcp_recvmsg().
    * we check for flags that are not of our interest and bail out
    * if we find any.
    */
   if (unlikely(flags & MSG_ERRQUEUE) || flags & MSG_OOB || flags & MSG_PEEK)
      goto out;

   /* special case on Linux.
    * https://lwn.net/Articles/495304/
    */
   if (unlikely(tp->repair))
      goto out;

   /* we bail out until the userspace process reads the urgent data */
   if (tp->urg_data)
      goto out;

   if (skb_queue_empty(&sk->sk_receive_queue) || !ctx->entry[0])
      goto out;

   skb = skb_peek_tail(&sk->sk_receive_queue);

   /* while we don't have sparse buffer pattern matching in Lua,
    * we have to drop non-linears skbs processing.
    */
   if (skb_is_nonlinear(skb)) {
      pp_warn("non linear processing is not implemented");
      goto out;
   }

   hdr = tcp_hdr(skb);

   /* this is an empiric assertion, as data may be received via ACKs as well.
    * right now we are considering only GRO-on cases, where a typical HTTP
    * request is reassembled before it arrives here.
    */
   if (unlikely(!hdr->psh))
      goto out;

   /* Lua guarantees some stack space */
   lua_pushcfunction(L, __doprocess);
   lua_pushlightuserdata(L, (void *) skb);

   perr = lua_pcall(L, 1, 1, 0);
   if (perr) {
      pp_pcall(perr, lua_tostring(L, -1));
      lua_pop(L, 1);
      goto out;
   }

   if (lua_toboolean(L, -1) == false) {
      lua_pop(L, 1);
      goto bad;
   }

out:
   release_sock(sk);
   return sys->recvmsg(sk, msg, len, nonblock, flags, addr_len);

bad:
   release_sock(sk);
   return -ECONNREFUSED;
}

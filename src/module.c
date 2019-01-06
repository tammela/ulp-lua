#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt "\n"
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/net.h>
#include <linux/socket.h>
#include <net/tcp.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <luadata.h>

#include "allocator.h"
#include "pretty.h"

/* #include <uapi/linux/ulp_lua.h> */
#define SS_LUA_LOADSCRIPT      (1)
#define SS_LUA_ENTRYPOINT      (2)
#define SS_LUA_RECVBUFFSZ      (3)

#define SOL_LUA                999
#define TCP_ULP_LUA            2

/* We must define SOL_LUA in /include/linux/socket.h */
/* We must define TCP_ULP_LUA in /include/net/tcp.h */

#define SS_ENTRYSZ (64)
#define SS_SCRIPTSZ (8192)

struct context {
   char entry[SS_ENTRYSZ];
};

static struct proto *sys;
static struct proto new;

static int ss_getsockopt(struct sock *sk, int level, int optname,
      char __user *optval, int __user *optlen);

static int ss_setsockopt(struct sock *sk, int level, int optname,
      char __user *optval, unsigned int optlen);

static int ss_recvmsg(struct sock *sk, struct msghdr *msg, size_t len,
      int nonblock, int flags, int *addr_len);

static struct sock *ss_accept(struct sock *sk, int flags, int *err, bool kern);

static void ss_close(struct sock *sk, long int timeout);

static inline lua_State *sk_ulp_data(struct sock *sk)
{
   return (lua_State *)inet_csk(sk)->icsk_ulp_data;
}

static inline struct context *sk_ctx(struct sock *sk)
{
   return *(struct context **)(lua_getextraspace(sk_ulp_data(sk)));
}

static void register_funcs(struct proto **skp)
{
   new = *sys;
   new.accept = ss_accept;
   new.recvmsg = ss_recvmsg;
   new.setsockopt = ss_setsockopt;
   new.getsockopt = ss_getsockopt;
   new.close = ss_close;
   *skp = &new;
}

static int sk_init(struct sock *sk)
{
   lua_State *L;
   struct context *ctx;
   struct context **area;

   if (sk->sk_family != AF_INET)
      return -ENOTSUPP;

   ctx = kmalloc(sizeof(struct context), GFP_KERNEL);
   if (unlikely(ctx == NULL))
      return -ENOMEM;

   ctx->entry[0] = '\0';

   L = luaL_newstate();
   if (unlikely(L == NULL))
      return -ENOMEM;

   luaL_openlibs(L);
   lua_setallocf(L, allocator, NULL);
   inet_csk(sk)->icsk_ulp_data = (void *)L;
   area = (struct context **)lua_getextraspace(L);
   *area = ctx;
   sys = sk->sk_prot;
   register_funcs(&sk->sk_prot);

   return 0;
}

static void ss_close(struct sock *sk, long int timeout)
{
   if (sk->sk_state == TCP_ESTABLISHED) {
      inet_csk(sk)->icsk_ulp_ops = NULL;
      module_put(THIS_MODULE);
   }

   if (sk->sk_state == TCP_LISTEN) {
      lua_close(sk_ulp_data(sk));
      kfree(sk_ctx(sk));
   }

   sk->sk_prot = sys;
   inet_csk(sk)->icsk_ulp_data = NULL;

   sys->close(sk, timeout);
}

static struct sock *ss_accept(struct sock *sk, int flags, int *err, bool kern)
{
   struct sock *reqsk = sys->accept(sk, flags, err, kern);

   if (reqsk == NULL)
      return NULL;

   try_module_get(THIS_MODULE);
   inet_csk(reqsk)->icsk_ulp_data = sk_ulp_data(sk);

   return reqsk;
}

static int ss_recvmsg(struct sock *sk, struct msghdr *msg, size_t len,
      int nonblock, int flags, int *addr_len)
{
   lua_State *L = sk_ulp_data(sk);
   struct context *ctx = sk_ctx(sk);
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

static int ss_setsockopt(struct sock *sk, int level, int optname,
      char __user *optval, unsigned int optlen)
{
   int err;

   if (level != SOL_LUA)
      return sys->setsockopt(sk, level, optname, optval, optlen);

   switch (optname) {
      case SS_LUA_LOADSCRIPT: {
         char *script;
         lua_State *L = sk_ulp_data(sk);

         if (sk->sk_state == TCP_ESTABLISHED)
            return -EINVAL;

         if (!optval || optlen > SS_SCRIPTSZ)
            return -EINVAL;

         script = vmalloc(optlen);
         if (unlikely(script == NULL))
            return -ENOMEM;

         err = copy_from_user(script, optval, optlen);
         if (unlikely(err)) {
            free(script);
            return -EFAULT;
         }

         if (luaL_loadbufferx(L, script, optlen, "lua", "t")
               || lua_pcall(L, 0, 0, 0)) {
            pr_err("%s", lua_tostring(L, -1));
            free(script);
            return -EINVAL;
         }

         break;
      }
      case SS_LUA_ENTRYPOINT: {
         struct context *ctx;

         if (!optval || optlen > SS_ENTRYSZ)
            return -EINVAL;

         ctx = sk_ctx(sk);
         err = copy_from_user(ctx->entry, optval, optlen);
         if (unlikely(err))
            return -EFAULT;

         break;
      }
      default:
        return -ENOPROTOOPT;
   }

   return 0;
}

static int ss_getsockopt(struct sock *sk, int level, int optname,
      char __user *optval, int __user *optlen)
{

   if (level != SOL_LUA)
      return sys->getsockopt(sk, level, optname, optval, optlen);

   switch (optname) {
      default:
         return -ENOPROTOOPT;
   }

   return 0;
}

static int ss_tcp_init(struct sock *sk)
{
   if (sk->sk_state == TCP_ESTABLISHED)
      return -EINVAL;

   return sk_init(sk);
}

static struct tcp_ulp_ops ss_tcpulp_ops __read_mostly = {
   .name          = "lua",
   .uid           = TCP_ULP_LUA,
   .user_visible  = true,
   .owner         = THIS_MODULE,
   .init          = ss_tcp_init
};

static int __init ss_tcp_register(void)
{
   tcp_register_ulp(&ss_tcpulp_ops);
   return 0;
}

static void __exit ss_tcp_unregister(void)
{
   tcp_unregister_ulp(&ss_tcpulp_ops);
}

module_init(ss_tcp_register);
module_exit(ss_tcp_unregister);

MODULE_LICENSE("GPL");

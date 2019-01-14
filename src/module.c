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

#include "ulp.h"
#include "syscalls.h"
#include "allocator.h"
#include "pretty.h"

static struct proto new;

static void register_funcs(struct proto **skp)
{
   new = *sys;
   new.accept = ulp_accept;
   new.recvmsg = ulp_recvmsg;
   new.setsockopt = ulp_setsockopt;
   new.getsockopt = ulp_getsockopt;
   new.close = ulp_close;
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

   L = lua_newstate(allocator, NULL);
   if (unlikely(L == NULL))
      return -ENOMEM;

   luaL_openlibs(L);
   inet_csk(sk)->icsk_ulp_data = (void *)L;
   area = (struct context **)lua_getextraspace(L);
   *area = ctx;
   sys = sk->sk_prot;
   register_funcs(&sk->sk_prot);

   return 0;
}

static int __ulp_init(struct sock *sk)
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
   .init          = __ulp_init
};

static int __init modinit(void)
{
   tcp_register_ulp(&ss_tcpulp_ops);
   return 0;
}

static void __exit modexit(void)
{
   tcp_unregister_ulp(&ss_tcpulp_ops);
}

module_init(modinit);
module_exit(modexit);
MODULE_AUTHOR("Pedro Tammela <pctammela@gmail.com>");
MODULE_LICENSE("GPL");

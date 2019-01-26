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
#include "pool.h"
#include "syscalls.h"
#include "allocator.h"
#include "pretty.h"

struct proto *sys;

static struct proto newprot;

static void register_funcs(struct proto **skp)
{
   newprot.accept = ulp_accept;
   newprot.close = ulp_close;
   newprot.getsockopt = ulp_getsockopt;
   newprot.recvmsg = ulp_recvmsg;
   newprot.setsockopt = ulp_setsockopt;

   *skp = &newprot;
}

static int sk_init(struct sock *sk)
{
   if (sk->sk_family != AF_INET)
      return -ENOTSUPP;

   /* save the original state */
   sys = sk->sk_prot;
   newprot = *(sk->sk_prot);

   register_funcs(&sk->sk_prot);

   pool_init(ULP_POOLSZ);

   return 0;
}

static int ulp_lua_init(struct sock *sk)
{
   if (sk->sk_state == TCP_ESTABLISHED)
      return -EINVAL;

   return sk_init(sk);
}

static struct tcp_ulp_ops ss_tcpulp_ops __read_mostly = {
   .name          = "lua",
   .owner         = THIS_MODULE,
   .init          = ulp_lua_init
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
MODULE_ALIAS_TCP_ULP("lua");
MODULE_LICENSE("GPL");

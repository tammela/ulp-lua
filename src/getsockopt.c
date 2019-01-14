#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt "\n"
#include <linux/kernel.h>

#include <linux/net.h>
#include <linux/socket.h>
#include <net/tcp.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "ulp.h"
#include "syscalls.h"

int ulp_getsockopt(struct sock *sk, int level, int optname,
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


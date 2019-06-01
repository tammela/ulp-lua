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
   int val, len;
   struct pool *pool;

   if (level != SOL_LUA)
      return sys->getsockopt(sk, level, optname, optval, optlen);

   /*
    * ULP_POOLSIZE option only valid for LISTENING sockets
    */
   if (sk->sk_state != TCP_LISTEN)
      return -ENOPROTOOPT;

   pool = inet_csk(sk)->icsk_ulp_data;

   if (get_user(len, optlen))
      return -EFAULT;

   len = min_t(unsigned int, len, sizeof(int));

   if (len < 0)
      return -EINVAL;

   switch (optname) {
      case ULP_POOLSIZE: {
         val = pool_getmaxsize(pool);

         break;
      }
      default:
         return -ENOPROTOOPT;
   }

   if (put_user(len, optlen))
      return -EFAULT;
   if (copy_to_user(optval, &val, len))
      return -EFAULT;

   return 0;
}


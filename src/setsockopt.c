#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt "\n"
#include <linux/kernel.h>

#include <linux/net.h>
#include <linux/socket.h>
#include <net/tcp.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "ulp.h"
#include "pool.h"
#include "pretty.h"
#include "syscalls.h"

int ulp_setsockopt(struct sock *sk, int level, int optname,
      char __user *optval, unsigned int optlen)
{
   int err;
   struct pool *pool;

#ifdef HAS_TLS
   if (level == SOL_TLS) {
      void *ptr = sk_ulp_data(sk);

      inet_csk(sk)->icsk_ulp_data = ((struct pool_entry *) ptr)->tc;
      err = sys->setsockopt(sk, level, optname, optval, optlen);
      /* restore our context */
      inet_csk(sk)->icsk_ulp_data = ptr;

      return err;
   }
#endif

   if (level != SOL_LUA)
      return sys->setsockopt(sk, level, optname, optval, optlen);

   pool = sk_listener_ulp_data(sk);
   BUG_ON(pool == NULL);

   switch (optname) {
      case ULP_LOADSCRIPT: {
         char *script;

         if (!optval || optlen > ULP_SCRIPTSZ)
            return -EINVAL;

         script = vmalloc(optlen);
         if (unlikely(script == NULL))
            return -ENOMEM;

         err = copy_from_user(script, optval, optlen);
         if (unlikely(err)) {
            vfree(script);
            return -EFAULT;
         }

         err = pool_scatter_script(pool, (const char *)script, optlen);
         vfree(script);
         if (err)
            return err;

         break;
      }
      case ULP_ENTRYPOINT: {
         char *entry;

         if (!optval || optlen > ULP_ENTRYSZ)
            return -EINVAL;

         entry = vmalloc(optlen);
         if (unlikely(entry == NULL))
            return -ENOMEM;

         err = copy_from_user(entry, optval, optlen);
         if (unlikely(err)) {
            vfree(entry);
            return -EFAULT;
         }

         err = pool_scatter_entry(pool, (const char *)entry, optlen);
         vfree(entry);
         if (unlikely(err))
            return err;

         break;
      }
      default:
        return -ENOPROTOOPT;
   }

   return 0;
}

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

   if (level != SOL_LUA)
      return sys->setsockopt(sk, level, optname, optval, optlen);

   switch (optname) {
      case SS_LUA_LOADSCRIPT: {
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

         err = pool_scatter((const char *)script, optlen);
         vfree(script);
         if (err)
            return err;

         break;
      }
      case SS_LUA_ENTRYPOINT: {
         struct context *ctx;

         if (!optval || optlen > ULP_ENTRYSZ)
            return -EINVAL;

         ctx = sk_ulp_ctx(sk);
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

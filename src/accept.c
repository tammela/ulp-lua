#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt "\n"
#include <linux/kernel.h>

#include <linux/net.h>
#include <linux/socket.h>
#include <net/tcp.h>

#include "ulp.h"
#include "pool.h"
#include "pretty.h"
#include "syscalls.h"

struct sock *ulp_accept(struct sock *sk, int flags, int *err, bool kern)
{
   int ret;
   struct pool_entry *entry;
   struct pool *pool = sk_listener_ulp_data(sk);
   struct sock *reqsk = sys->accept(sk, flags, err, kern);

   if (reqsk == NULL)
      return NULL;

   try_module_get(THIS_MODULE);

   /* this should never happen! */
   pool_lock(pool);
   if (unlikely(pool_empty(pool))) {
      ret = pool_resize(pool, pool_size(pool) + 1);
      if (ret != 0) {
         pp_errno(ret, "caught errno");
         pool_unlock(pool);
         return NULL;
      }
   }
   pool_unlock(pool);

   entry = pool_pop(pool);
   WARN_ON(entry == NULL);
   if (entry == NULL)
      return NULL;

   ret = sk_set_ulp_data(reqsk, CONNECTION, entry);
   if (ret) {
      sys->close(reqsk, 0);
      reqsk = NULL;
   }

   return reqsk;
}

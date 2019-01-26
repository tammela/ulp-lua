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
   struct sock *reqsk = sys->accept(sk, flags, err, kern);

   if (reqsk == NULL)
      return NULL;

   try_module_get(THIS_MODULE);

   if (unlikely(pool_empty())) {
      ret = pool_resize(pool_size() + 1);
      if (ret != 0) {
         pp_errno(ret, "caught errno");
         return NULL;
      }
   }

   inet_csk(reqsk)->icsk_ulp_data = pool_pop();

   return reqsk;
}

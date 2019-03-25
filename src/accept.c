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
   int ret = 0;
   struct sock *reqsk = sys->accept(sk, flags, err, kern);

   if (reqsk == NULL)
      return NULL;

   try_module_get(THIS_MODULE);

   /* this should never happen! */
   if (unlikely(pool_empty())) {
      ret = pool_resize(pool_size() + 1);
      if (ret != 0) {
         pp_errno(ret, "caught errno");
         return NULL;
      }
   }

#ifdef HAS_TLS
   ret = ((int (*)(struct sock *, const char *))addr_tcp_set_ulp)(reqsk, "tls");
   if (ret) {
      pp_errno(ret, "caught errno in TLS ulp");
      *err = ret;
   }
#endif

   inet_csk(reqsk)->icsk_ulp_data = pool_pop(inet_csk(reqsk)->icsk_ulp_data);

   return reqsk;
}

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
   struct pool_entry *entry;
   struct pool *pool = sk_listener_ulp_data(sk);
   struct sock *reqsk = sys->accept(sk, flags, err, kern);
#ifdef HAS_TLS
   const struct tcp_ulp_ops *ops;
#endif

   if (reqsk == NULL)
      return NULL;

#ifdef HAS_TLS
   ops = inet_csk(reqsk)->icsk_ulp_ops;
   /* fake a clean ulp socket */
   inet_csk(reqsk)->icsk_ulp_ops = NULL;
   ret = ((int (*)(struct sock *, const char *))addr_tcp_set_ulp)(reqsk, "tls");
   if (ret) {
      pp_errno(ret, "caught errno in TLS ulp");
      *err = ret;
   }
   inet_csk(reqsk)->icsk_ulp_ops = ops;
#endif

   entry = pool_pop(pool, inet_csk(reqsk)->icsk_ulp_data);
   if (entry == NULL)
      goto close;

   ret = sk_set_ulp_data(reqsk, CONNECTION, entry);
   if (ret) {
      goto close;
   }

   return reqsk;
close:
   inet_csk(reqsk)->icsk_ulp_ops = NULL;
   sys->close(reqsk, 0);
   return NULL;
}

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
   struct pool *pool;
   struct sock *reqsk;

   pool = sk_listener_ulp_data(sk);
   reqsk = sys->accept(sk, flags, err, kern);

   if (reqsk == NULL)
      return NULL;

   entry = pool_pop(pool);
   WARN_ON(entry == NULL);
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

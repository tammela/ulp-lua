#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt "\n"
#include <linux/kernel.h>

#include <linux/net.h>
#include <linux/socket.h>
#include <net/tcp.h>

#include "ulp.h"
#include "syscalls.h"

struct sock *ulp_accept(struct sock *sk, int flags, int *err, bool kern)
{
   struct sock *reqsk = sys->accept(sk, flags, err, kern);

   if (reqsk == NULL)
      return NULL;

   try_module_get(THIS_MODULE);
   inet_csk(reqsk)->icsk_ulp_data = sk_ulp_data(sk);

   return reqsk;
}

#include <linux/kernel.h>

#include <linux/net.h>
#include <linux/socket.h>
#include <net/tcp.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "ulp.h"
#include "pool.h"
#include "syscalls.h"

void ulp_close(struct sock *sk, long int timeout)
{
   if (sk->sk_state == TCP_ESTABLISHED) {
      inet_csk(sk)->icsk_ulp_ops = NULL;
      module_put(THIS_MODULE);
   }

   if (sk->sk_state == TCP_LISTEN) {
      pool_recycle(sk_ulp_data(sk));
      kfree(sk_ulp_ctx(sk));
   }

   sk->sk_prot = sys;
   inet_csk(sk)->icsk_ulp_data = NULL;
   sys->close(sk, timeout);
}

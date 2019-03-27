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
   pool_recycle(sk_ulp_data(sk));

   /* clean up the state pool even if killed by a SIGKILL */
   if (current->flags & PF_EXITING || sk->sk_state == TCP_LISTEN) {
      pool_exit();
      goto out;
   }

   if (sk->sk_state != TCP_LISTEN) {
      inet_csk(sk)->icsk_ulp_ops = NULL;
   }

out:
   sk->sk_prot = sys;
   inet_csk(sk)->icsk_ulp_data = NULL;
   sys->close(sk, timeout);
}

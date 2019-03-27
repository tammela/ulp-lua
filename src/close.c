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
   struct ulp_data *data;

   data = inet_csk(sk)->icsk_ulp_data;

   // next few lines only for debug and will be removed
   BUG_ON(data == NULL);

   if (data->type == CONNECTION) {
      // We need to NULL icsk_ulp_ops to prevent module_put call inside tcp_ulp.c in kernel
      inet_csk(sk)->icsk_ulp_ops = NULL;
   }

   sk_cleanup_ulp_data(sk);
   sk->sk_prot = sys;
   sys->close(sk, timeout);
}

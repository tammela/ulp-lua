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
   struct ulp_data *data = inet_csk(sk)->icsk_ulp_data;
   bool is_data_state_sk_state_contradiction;

   // behavior changes with that lock
   lock_sock(sk);

   // next few lines only for debug and will be removed
   BUG_ON(data == NULL);
   BUG_ON(data->type == LISTENER && sk->sk_state != TCP_LISTEN);
   is_data_state_sk_state_contradiction = data->type == CONNECTION && !(sk->sk_state == TCP_ESTABLISHED || sk->sk_state == TCP_FIN_WAIT1 
                           || sk->sk_state == TCP_FIN_WAIT2 || sk->sk_state == TCP_CLOSE_WAIT 
                           || sk->sk_state == TCP_CLOSE || sk->sk_state == TCP_CLOSING 
                           || sk->sk_state == TCP_LAST_ACK);
   if (is_data_state_sk_state_contradiction) {
      printk("Connection socket but state is %d\n", sk->sk_state);
      BUG_ON(true);
   }

   if (data->type == CONNECTION) {
      // We need to NULL icsk_ulp_ops to prevent module_put call inside tcp_ulp.c in kernel
      inet_csk(sk)->icsk_ulp_ops = NULL;

      module_put(THIS_MODULE);
   }/* else {

      inet_csk(sk)->icsk_ulp_ops = NULL;
      module_put(THIS_MODULE);
   }*/

   sk_cleanup_ulp_data(sk);

   sk->sk_prot = sys;
   inet_csk(sk)->icsk_ulp_data = NULL;
   release_sock(sk);
   sys->close(sk, timeout);
}

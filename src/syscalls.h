#ifndef _SYSCALLS_H
#define _SYSCALLS_H

extern int ulp_getsockopt(struct sock *, int, int, char __user *, int __user *);
extern int ulp_recvmsg(struct sock *, struct msghdr *, size_t, int, int, int *);
extern int ulp_setsockopt(struct sock *, int, int, char __user *, unsigned int);
extern struct sock *ulp_accept(struct sock *, int, int *, bool);
extern void ulp_close(struct sock *, long int);

#endif

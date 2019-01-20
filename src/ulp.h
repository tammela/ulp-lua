#ifndef _ULP_H
#define _ULP_H

#include <linux/net.h>
#include <linux/socket.h>
#include <net/tcp.h>

#include <lua.h>
#include <lauxlib.h>

#include "conf.h"

/* We must define SOL_LUA in /include/linux/socket.h */
#define SOL_LUA                (999)

/* #include <uapi/linux/ulp_lua.h> */
#define SS_LUA_LOADSCRIPT      (1)
#define SS_LUA_ENTRYPOINT      (2)
#define SS_LUA_RECVBUFFSZ      (3)

struct context {
   char entry[ULP_ENTRYSZ];
};

static inline lua_State *sk_ulp_data(struct sock *sk)
{
   return (lua_State *)inet_csk(sk)->icsk_ulp_data;
}

static inline struct context *sk_ulp_ctx(struct sock *sk)
{
   return *(struct context **)(lua_getextraspace(sk_ulp_data(sk)));
}

#endif

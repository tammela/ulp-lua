#ifndef _ULP_H
#define _ULP_H

#include <linux/net.h>
#include <linux/socket.h>
#include <net/tcp.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/* We must define SOL_LUA in /include/linux/socket.h */
#define SOL_LUA                (999)

/* We must define TCP_ULP_LUA in /include/net/tcp.h */
#define TCP_ULP_LUA            (2)

/* maximum entry function name size in bytes */
#define SS_ENTRYSZ             (64)

/* maximum script size in bytes */
#define SS_SCRIPTSZ            (8192)

/* #include <uapi/linux/ulp_lua.h> */
#define SS_LUA_LOADSCRIPT      (1)
#define SS_LUA_ENTRYPOINT      (2)
#define SS_LUA_RECVBUFFSZ      (3)

struct context {
   char entry[SS_ENTRYSZ];
};

/* original system calls */
static struct proto *sys;

static inline lua_State *sk_ulp_data(struct sock *sk)
{
   return (lua_State *)inet_csk(sk)->icsk_ulp_data;
}

static inline struct context *sk_ulp_ctx(struct sock *sk)
{
   return *(struct context **)(lua_getextraspace(sk_ulp_data(sk)));
}

#endif

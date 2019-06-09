#ifndef _ULP_H
#define _ULP_H

#include <linux/net.h>
#include <linux/socket.h>
#include <net/tcp.h>

#include <lua.h>
#include <lauxlib.h>

#include "pool.h"
#include "conf.h"

/* FROM: NFLua */
/*
 * Copyright (C) 2017-2018 CUJO LLC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#define luaU_setenv(L, env, st) { \
	st **penv = (st **)lua_getextraspace(L); \
	*penv = env; }

#define luaU_getenv(L, st) (*((st **)lua_getextraspace(L)))
/* END */

/* We must define SOL_LUA in /include/linux/socket.h */
#define SOL_LUA                (999)

/* #include <uapi/linux/ulp_lua.h> */
#define ULP_LOADSCRIPT      (1)
#define ULP_ENTRYPOINT      (2)
#define ULP_POOLSIZE        (3)

struct context {
   char entry[ULP_ENTRYSZ];
};

extern unsigned long addr_tcp_set_ulp;

static inline struct pool_entry *sk_ulp_data(struct sock *sk)
{
   return (struct pool_entry *)inet_csk(sk)->icsk_ulp_data;
}

static inline void sk_cleanup_ulp_data(struct sock *sk)
{
   void *ptr = inet_csk(sk)->icsk_ulp_data;

   if (unlikely(ptr == NULL))
      return;

   /* listener */
   if (sk->sk_state == TCP_LISTEN)
      pool_exit((struct pool *)ptr);
   else
      pool_recycle((struct pool_entry *)ptr);

   inet_csk(sk)->icsk_ulp_data = NULL;
}

static inline struct context *sk_ulp_ctx(struct sock *sk)
{
   lua_State *L = sk_ulp_data(sk)->L;

   if (!L)
      return NULL;

   return luaU_getenv(L, struct context);
}

#endif

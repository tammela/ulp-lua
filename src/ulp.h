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

struct context {
   char entry[ULP_ENTRYSZ];
};

typedef enum {
   LISTENER = 1,
   CONNECTION
} ulp_data_type;

struct ulp_data {
   ulp_data_type type;
   union {
      struct pool_entry *entry;
      struct pool *pool;
   } data;
};

/*
 * We will remove ulp_data when will be confident
 * that we can distinguish listener socket
 * from connection socket only by sk_state field
 */
static inline int sk_set_ulp_data(struct sock *sk, ulp_data_type type, void *data)
{
   struct ulp_data *ulp_data;

   ulp_data = kmalloc(sizeof(struct ulp_data), GFP_KERNEL);
   if (unlikely(ulp_data == NULL))
      return -ENOMEM;

   ulp_data->type = type;

   if (type == LISTENER)
      ulp_data->data.pool = data;
   else
      ulp_data->data.entry = data;

   inet_csk(sk)->icsk_ulp_data = ulp_data;

   return 0;
}

static inline void sk_cleanup_ulp_data(struct sock *sk)
{
   struct ulp_data *data = inet_csk(sk)->icsk_ulp_data;
   WARN_ON(data == NULL);
   if (unlikely(data == NULL))
      return;

   if (data->type == LISTENER)
      pool_exit(data->data.pool);
   else if (data->type == CONNECTION)
      pool_recycle(data->data.entry);
   else
      BUG_ON(true);

   inet_csk(sk)->icsk_ulp_data = NULL;
   kfree(data);
}

static inline struct pool_entry *sk_conn_ulp_data(struct sock *sk)
{
   struct ulp_data *data = inet_csk(sk)->icsk_ulp_data;
   BUG_ON(data == NULL);
   BUG_ON(data->type != CONNECTION);
   return data->data.entry;
}

static inline struct pool *sk_listener_ulp_data(struct sock *sk)
{
   struct ulp_data *data = inet_csk(sk)->icsk_ulp_data;
   BUG_ON(data == NULL);
   BUG_ON(data->type != LISTENER);
   return data->data.pool;
}

static inline struct context *sk_ulp_ctx(struct sock *sk)
{
   lua_State *L = sk_conn_ulp_data(sk)->L;

   if (!L)
      return NULL;

   return luaU_getenv(L, struct context);
}

#endif

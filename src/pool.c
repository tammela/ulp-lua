#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt "\n"
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/slab.h>

#include <net/tls.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "ulp.h"
#include "pretty.h"
#include "pool.h"
#include "allocator.h"

void pool_lock(struct pool *pool)
{
   spin_lock(&pool->lock);
}

void pool_unlock(struct pool *pool)
{
   spin_unlock(&pool->lock);
}

static inline void __pool_list_add(struct pool *pool, struct list_head *new)
{
   list_add(new, &pool->list);
   pool->size++;
}

static inline void __pool_list_del(struct pool *pool, struct list_head *entry)
{
   list_del(entry);
   pool->size--;
}

static int __pool_add(struct pool *pool, int n)
{
   struct pool_entry *entry;
   struct context *ctx;
   int i;

   for (i = 0; i < n; i++) {
      entry = kmalloc(sizeof(struct pool_entry), GFP_KERNEL);
      if (unlikely(entry == NULL))
         goto error;

      entry->pool = pool;

      entry->L = lua_newstate(allocator, NULL);
      if (unlikely(entry->L == NULL)) {
         kfree(entry);
         goto error;
      }

      ctx = kmalloc(sizeof(struct context), GFP_KERNEL);
      if (unlikely(ctx == NULL)) {
         lua_close(entry->L);
         kfree(entry);
         goto error;
      }

      ctx->entry[0] = '\0';

      /* setup Lua state */
      luaU_setenv(entry->L, ctx, struct context);
      luaL_openlibs(entry->L);
      lua_gc(entry->L, LUA_GCSETPAUSE, ULP_LUAGCPAUSE);

      __pool_list_add(pool, &entry->head);
   }

   return 0;

error:
   return -ENOMEM;
}

static int __pool_del(struct pool *pool, int n)
{
   struct pool_entry *entry;
   struct context *ctx;
   int i;

   for (i = 0; i < n; i++) {
      entry = list_first_entry(&pool->list, struct pool_entry, head);
      ctx = luaU_getenv(entry->L, struct context);

      lua_close(entry->L);

      __pool_list_del(pool, &entry->head);

      kfree(entry);
      kfree(ctx);
   }

   return 0;
}

struct pool *pool_init(int size)
{
   struct pool *pool;
   int err;

   pool = kmalloc(sizeof(struct pool), GFP_KERNEL);
   if (unlikely(pool == NULL))
      return NULL;

   INIT_LIST_HEAD(&pool->list);
   pool->size = 0;
   spin_lock_init(&pool->lock);

   err = __pool_add(pool, size);
   if (unlikely(err != 0)) {
      pp_errno(err, "couldn't create pool");
      kfree(pool);
      return NULL;
   }

   return pool;
}

void pool_exit(struct pool *pool)
{
   pool_lock(pool);
   __pool_del(pool, pool->size);
   pool_unlock(pool);
   kfree(pool);
}

int pool_empty(struct pool *pool)
{
   return list_empty(&pool->list);
}

void pool_recycle(struct pool_entry *entry)
{
   if (unlikely(entry == NULL))
      return;

   pool_lock(entry->pool);
   __pool_list_add(entry->pool, &entry->head);
   pool_unlock(entry->pool);
}

int pool_scatter_script(struct pool *pool, const char *script, size_t sz)
{
   struct pool_entry *bkt;
   int err = 0;

   pool_lock(pool);

   list_for_each_entry(bkt, &pool->list, head) {
      err = luaL_loadbufferx(bkt->L, script, sz, "lua", "t");
      if (unlikely(err))
         goto bad;

      err = lua_pcall(bkt->L, 0, 0, 0);
      if (unlikely(err)) {
         pp_pcall(err, "caught error in scatter script" );
         goto bad;
      }
   }

   pool_unlock(pool);
   return 0;

bad:
   pool_unlock(pool);
   pp_pcall(err, lua_tostring(bkt->L, -1));
   return -EINVAL;
}

int pool_scatter_entry(struct pool *pool, const char *entry, size_t sz)
{
   struct pool_entry *bkt;
   struct context *ctx;
   int err = 0;

   pool_lock(pool);

   list_for_each_entry(bkt, &pool->list, head) {
      ctx = luaU_getenv(bkt->L, struct context);
      if (unlikely(!ctx)) {
         err = -EINVAL;
         goto exit;
      }

      memcpy(ctx->entry, entry, sz);
   }

exit:
   pool_unlock(pool);
   return err;
}

struct pool_entry *pool_pop(struct pool *pool, void *data)
{
   struct pool_entry *entry;

   pool_lock(pool);

   if (unlikely(pool_empty(pool))) {
      entry = NULL;
      goto unlock;
   }

   entry = list_first_entry(&pool->list, struct pool_entry, head);
#ifdef HAS_TLS
   entry->tc = data;
#endif
   __pool_list_del(pool, &entry->head);

unlock:
   pool_unlock(pool);
   return entry;
}

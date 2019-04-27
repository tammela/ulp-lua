#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt "\n"
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/slab.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "ulp.h"
#include "pretty.h"
#include "pool.h"
#include "allocator.h"

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

   BUG_ON(n > pool->size);

   for (i = 0; i < n; i++) {
      entry = list_first_entry(&pool->list, struct pool_entry, head);
      BUG_ON(list_empty(&pool->list));
      ctx = luaU_getenv(entry->L, struct context);
      kfree(ctx);
      lua_close(entry->L);
      __pool_list_del(pool, &entry->head);
      kfree(entry);
   }

   return 0;
}

struct pool *pool_init(int size)
{
   int err;
   struct pool *pool;

   pool = kmalloc(sizeof(struct pool), GFP_KERNEL);
   if (unlikely(pool == NULL))
      return NULL;

   INIT_LIST_HEAD(&pool->list);
   pool->size = 0;
   spin_lock_init(&pool->lock);

   err = __pool_add(pool, size);
   if (unlikely(err != 0)) {
      kfree(pool);
      return NULL;
   }

   return pool;
}

void pool_exit(struct pool *pool)
{
   __pool_del(pool, pool->size);
   kfree(pool);
}

int pool_size(struct pool *pool)
{
   return pool->size;
}

int pool_empty(struct pool *pool)
{
   return list_empty(&pool->list);
}

void pool_lock(struct pool *pool)
{
   spin_lock(&pool->lock);
}

void pool_unlock(struct pool *pool)
{
   spin_unlock(&pool->lock);
}

int pool_resize(struct pool *pool, int resize)
{
   pp_warn("resizing pool");

   if (unlikely(resize == pool->size))
      return 0;

   if (resize > pool->size)
      return __pool_add(pool, resize - pool->size);

   if (resize < pool->size)
      return __pool_del(pool, pool->size - resize);

   return 0;
}

void pool_recycle(struct pool_entry *entry)
{
   if (unlikely(entry == NULL))
      return;

   spin_lock(&entry->pool->lock);
   __pool_list_add(entry->pool, &entry->head);
   spin_unlock(&entry->pool->lock);
}

int pool_scatter_script(struct pool *pool, const char *script, size_t sz)
{
   struct pool_entry *bkt;
   int err = 0;

   spin_lock(&pool->lock);

   list_for_each_entry(bkt, &pool->list, head) {
      err = luaL_loadbufferx(bkt->L, script, sz, "lua", "t");
      if (err)
         goto bad;

      err = lua_pcall(bkt->L, 0, 0, 0);
      if (err)
         goto bad;
   }

   spin_unlock(&pool->lock);
   return 0;

bad:
   pp_pcall(err, lua_tostring(bkt->L, -1));
   spin_unlock(&pool->lock);
   return -EINVAL;
}

int pool_scatter_entry(struct pool *pool, const char *entry, size_t sz)
{
   struct pool_entry *bkt;
   struct context *ctx;
   int err = 0;

   spin_lock(&pool->lock);

   list_for_each_entry(bkt, &pool->list, head) {
      ctx = luaU_getenv(bkt->L, struct context);
      if (!ctx) {
         err = -EINVAL;
         goto exit;
      }

      memcpy(ctx->entry, entry, sz);
   }

exit:
   spin_unlock(&pool->lock);
   return err;
}

struct pool_entry *pool_pop(struct pool *pool)
{
   struct pool_entry *entry;

   spin_lock(&pool->lock);

   if (unlikely(pool_empty(pool))) {
      spin_unlock(&pool->lock);
      return NULL;
   }

   entry = list_first_entry(&pool->list, struct pool_entry, head);
   __pool_list_del(pool, &entry->head);

   spin_unlock(&pool->lock);

   return entry;
}

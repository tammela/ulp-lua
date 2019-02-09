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

static LIST_HEAD(pool_lst);
static DEFINE_SPINLOCK(pool_lock);
static int poolsz = 0;

static int __pool_add(int n)
{
   struct pool_entry *entry;
   struct context *ctx;
   int i;

   for (i = 0; i < n; i++) {
      entry = kmalloc(sizeof(struct pool_entry), GFP_KERNEL);
      if (unlikely(entry == NULL))
         goto error;

      entry->L = lua_newstate(allocator, NULL);
      if (unlikely(entry->L == NULL))
         goto error;

      ctx = kmalloc(sizeof(struct context), GFP_KERNEL);
      if (unlikely(ctx == NULL))
         goto error;

      ctx->entry[0] = '\0';

      /* setup Lua state */
      luaU_setenv(entry->L, ctx, struct context);
      luaL_openlibs(entry->L);
      lua_gc(entry->L, LUA_GCSETPAUSE, ULP_LUAGCPAUSE);

      list_add(&entry->head, &pool_lst);
      poolsz++;
   }

   return 0;

error:
   pool_exit();
   return -ENOMEM;
}

static int __pool_del(int n)
{
   struct pool_entry *entry;
   struct context *ctx;
   int i;

   for (i = 0; i < n; i++) {
      entry = list_first_entry(&pool_lst, struct pool_entry, head);
      ctx = luaU_getenv(entry->L, struct context);
      kfree(ctx);
      lua_close(entry->L);
      list_del(&entry->head);
      kfree(entry);
      poolsz--;
   }

   return 0;
}

int pool_init(int size)
{
   int err;

   err = __pool_add(size);
   if (unlikely(err != 0))
      return err;

   poolsz = size;

   return 0;
}

void pool_exit(void)
{
   __pool_del(poolsz);
}

int pool_size(void)
{
   return poolsz;
}

int pool_empty(void)
{
   return list_empty(&pool_lst);
}

int pool_resize(int resize)
{
   pp_warn("resizing pool");

   if (unlikely(resize == poolsz))
      return 0;

   if (resize > poolsz)
      return __pool_add(resize - poolsz);

   if (resize < poolsz)
      return __pool_del(poolsz - resize);

   return 0;
}

void pool_recycle(struct pool_entry *entry)
{
   spin_lock(&pool_lock);
   list_add(&entry->head, &pool_lst);
   spin_unlock(&pool_lock);
}

int pool_scatter_script(const char *script, size_t sz)
{
   struct pool_entry *bkt;
   int err = 0;

   list_for_each_entry(bkt, &pool_lst, head) {
      err = luaL_loadbufferx(bkt->L, script, sz, "lua", "t");
      if (err)
         goto bad;

      err = lua_pcall(bkt->L, 0, 0, 0);
      if (err)
         goto bad;
   }

   return 0;

bad:
   pp_pcall(err, lua_tostring(bkt->L, -1));
   return -EINVAL;
}

int pool_scatter_entry(const char *entry, size_t sz)
{
   struct pool_entry *bkt;
   struct context *ctx;

   list_for_each_entry(bkt, &pool_lst, head) {
      ctx = luaU_getenv(bkt->L, struct context);
      if (!ctx)
         return -EINVAL;

      memcpy(ctx->entry, entry, sz);
   }

   return 0;
}

struct pool_entry *pool_pop(void)
{
   struct pool_entry *entry;

   spin_lock(&pool_lock);
   entry = list_first_entry(&pool_lst, struct pool_entry, head);
   list_del(&entry->head);
   spin_unlock(&pool_lock);

   return entry;
}

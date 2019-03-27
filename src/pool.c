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

static inline void __pool_list_add(struct list_head *new)
{
   list_add(new, &pool_lst);
   poolsz++;
}

static inline void __pool_list_del(struct list_head *entry)
{
   list_del(entry);
   poolsz--;
}

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

      __pool_list_add(&entry->head);
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

   BUG_ON(n > poolsz);

   for (i = 0; i < n; i++) {
      entry = list_first_entry(&pool_lst, struct pool_entry, head);
      BUG_ON(list_empty(&pool_lst));
      ctx = luaU_getenv(entry->L, struct context);
      kfree(ctx);
      lua_close(entry->L);
      __pool_list_del(&entry->head);
      kfree(entry);
   }

   return 0;
}

int pool_init(int size)
{
   int err;

   spin_lock(&pool_lock);

   BUG_ON(poolsz != 0);

   err = __pool_add(size);
   if (unlikely(err != 0))
      goto exit;

   BUG_ON(poolsz != size);

exit:
   spin_unlock(&pool_lock);
   return err;
}

void pool_exit(void)
{
   spin_lock(&pool_lock);
   __pool_del(poolsz);
   spin_unlock(&pool_lock);
}

int pool_size(void)
{
   return poolsz;
}

int pool_empty(void)
{
   return list_empty(&pool_lst);
}

static int pool_resize(int resize)
{
   int err = 0;

   pp_warn("resizing pool");

   if (unlikely(resize == poolsz))
      goto exit;

   if (resize > poolsz) {
      err = __pool_add(resize - poolsz);
      goto exit;
   }

   if (resize < poolsz) {
      err = __pool_del(poolsz - resize);
      goto exit;
   }

exit:
   return err;
}

void pool_recycle(struct pool_entry *entry)
{
   if (unlikely(entry == NULL))
      return;

   spin_lock(&pool_lock);
   __pool_list_add(&entry->head);
/*
   if (poolsz > 2 * ULP_POOLSZ) {
      pool_resize(ULP_POOLSZ);
   }
*/
   spin_unlock(&pool_lock);
}

int pool_scatter_script(const char *script, size_t sz)
{
   struct pool_entry *bkt;
   int err = 0;

   spin_lock(&pool_lock);

   list_for_each_entry(bkt, &pool_lst, head) {
      err = luaL_loadbufferx(bkt->L, script, sz, "lua", "t");
      if (err)
         goto bad;

      err = lua_pcall(bkt->L, 0, 0, 0);
      if (err)
         goto bad;
   }

   spin_unlock(&pool_lock);
   return 0;

bad:
   pp_pcall(err, lua_tostring(bkt->L, -1));
   spin_unlock(&pool_lock);
   return -EINVAL;
}

int pool_scatter_entry(const char *entry, size_t sz)
{
   struct pool_entry *bkt;
   struct context *ctx;
   int err = 0;

   spin_lock(&pool_lock);

   list_for_each_entry(bkt, &pool_lst, head) {
      ctx = luaU_getenv(bkt->L, struct context);
      if (!ctx) {
         err = -EINVAL;
         goto exit;
      }

      memcpy(ctx->entry, entry, sz);
   }

exit:
   spin_unlock(&pool_lock);
   return err;
}

struct pool_entry *pool_pop(void)
{
   struct pool_entry *entry;
   int ret;

   spin_lock(&pool_lock);

   /* this should never happen! */
   if (unlikely(pool_empty())) {
      BUG_ON(pool_empty() && (pool_size() != 0));
      ret = pool_resize(pool_size() + ULP_POOLSZ);
      if (ret != 0) {
         pp_errno(ret, "caught errno");
         return NULL;
      }
   }

   BUG_ON(poolsz == 0);
   entry = list_first_entry(&pool_lst, struct pool_entry, head);
   __pool_list_del(&entry->head);

   spin_unlock(&pool_lock);

   return entry;
}

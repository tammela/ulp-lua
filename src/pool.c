#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt "\n"
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "ulp.h"
#include "pretty.h"
#include "pool.h"
#include "allocator.h"

struct pool_entry {
   lua_State *L;
   struct list_head head;
};

static LIST_HEAD(pool_lst);
static int poolsz;

static int __pool_add(int n)
{
   struct pool_entry *entry;
   struct context *ctx;
   struct context **area;
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

      luaL_openlibs(entry->L);

      area = (struct context **)lua_getextraspace(entry->L);
      *area = ctx;

      list_add(&entry->head, &pool_lst);
   }

   return 0;

error:
   pool_exit();
   return -ENOMEM;
}

static int __pool_del(int n)
{
   struct pool_entry *entry;
   int i;

   for (i = 0; i < n; i++) {
      entry = list_first_entry(&pool_lst, struct pool_entry, head);
      list_del(&entry->head);
      kfree(entry);
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
   struct pool_entry *bucket;

   list_for_each_entry(bucket, &pool_lst, head) {
      kfree(bucket);
   }
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

void pool_recycle(lua_State *L)
{
   struct pool_entry *entry = container_of((void *)L, struct pool_entry, L);

   list_add(&entry->head, &pool_lst);
}

lua_State *pool_pop(void)
{
   struct pool_entry *entry =
      list_first_entry(&pool_lst, struct pool_entry, head);

   list_del(&entry->head);

   return entry->L;
}

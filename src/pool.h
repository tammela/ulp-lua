#ifndef _POOL_H
#define _POOL_H

struct pool {
   struct list_head list;
   int size;
   spinlock_t lock;
};

struct pool_entry {
   lua_State *L;
   struct pool *pool;
   struct list_head head;
};

extern struct pool *pool_init(int);

extern void pool_exit(struct pool *pool);

extern int pool_empty(struct pool *pool);

extern void pool_lock(struct pool *pool);

extern void pool_unlock(struct pool *pool);

extern int pool_size(struct pool *pool);

extern int pool_resize(struct pool *pool, int resize);

extern void pool_recycle(struct pool_entry *);

extern int pool_scatter_script(struct pool *pool, const char *, size_t);

extern int pool_scatter_entry(struct pool *pool, const char *, size_t);

extern struct pool_entry *pool_pop(struct pool *pool);

#endif

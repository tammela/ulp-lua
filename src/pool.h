#ifndef _POOL_H
#define _POOL_H

struct pool {
   struct list_head list;
   /* number of elements that assigned to pool */
   int maxsize;
   /* number of elements that currently in pool */
   int size;
   spinlock_t lock;
};

struct pool_entry {
#ifdef HAS_TLS
   struct tls_context *tc;
#endif
   lua_State *L;
   struct pool *pool;
   struct list_head head;
};

extern struct pool *pool_init(int);

extern void pool_exit(struct pool *);

extern int pool_empty(struct pool *);

extern void pool_lock(struct pool *);

extern void pool_unlock(struct pool *);

extern void pool_recycle(struct pool_entry *);

extern int pool_scatter_script(struct pool *, const char *, size_t);

extern int pool_scatter_entry(struct pool *, const char *, size_t);

extern int pool_setmaxsize(struct pool *, int);

extern int pool_getmaxsize(struct pool *);

extern struct pool_entry *pool_pop(struct pool *, void *);

#endif

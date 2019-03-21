#ifndef _POOL_H
#define _POOL_H

struct pool_entry {
#ifdef CONFIG_TLS
   struct tls_context tc; /* always first member! */
#endif
   lua_State *L;
   struct list_head head;
};

extern int pool_init(int);

extern void pool_exit(void);

extern int pool_empty(void);

extern int pool_size(void);

extern int pool_resize(int);

extern void pool_recycle(struct pool_entry *);

extern int pool_scatter_script(const char *, size_t);

extern int pool_scatter_entry(const char *, size_t);

extern struct pool_entry *pool_pop(void *);

#endif

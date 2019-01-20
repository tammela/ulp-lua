#ifndef _POOL_H
#define _POOL_H

extern int pool_init(int);

extern void pool_exit(void);

extern int pool_empty(void);

extern int pool_size(void);

extern int pool_resize(int);

extern void pool_recycle(lua_State *);

extern lua_State *pool_pop(void);

#endif

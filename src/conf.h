#ifndef _CONF_H
#define _CONF_H

/* maximum entry function name size in bytes */
#define ULP_ENTRYSZ             (64)

/* maximum script size in bytes */
#define ULP_SCRIPTSZ            (8192)

/* initial lua_State pool size */
#define ULP_POOLSZ              (16)

/* initial Lua'a GC pause value */
#define ULP_LUAGCPAUSE          (100)

#endif

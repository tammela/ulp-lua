#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt "\n"
#include <linux/kernel.h>

#include <lua.h>
#include <lualib.h>

void __pp_pcall(int errnum, const char *msg, const char *file, const char* func, int line)
{
   pr_err("%s:%d [%s()] --> %s", file, line, func, msg);
   switch (errnum) {
      case LUA_OK:
         pr_err("%s:%d [%s()] --> Lua returned success", file, line, func);
         break;
      case LUA_ERRRUN:
         pr_err("%s:%d [%s()] --> Lua returned a runtime error",
               file, line, func);
         break;
      case LUA_ERRMEM:
         pr_err("%s:%d [%s()] --> Lua returned a memory allocation error",
               file, line, func);
         break;
      case LUA_ERRERR:
         pr_err("%s:%d [%s()] --> Lua returned an error"
               "while running the message handler",
               file, line, func);
         break;
      case LUA_ERRGCMM:
         pr_err("%s:%d [%s()] --> Lua returned an error"
               "while running a __gc metamethod",
               file, line, func);
         break;
      default:
         pr_err("%s:%d [%s()] --> Unknown error code."
               "Use this function on lua_pcall() return only.",
               file, line, func);
         break;
   }
}

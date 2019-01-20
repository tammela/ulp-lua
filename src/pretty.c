#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt "\n"
#include <linux/kernel.h>

#include <lua.h>
#include <lualib.h>

#define PP_PREFIX "%s:%d [%s()] -->"

void __pp_pcall(int errnum, const char *msg, const char *file, const char* func, int line)
{
   pr_err(PP_PREFIX" %s", file, line, func, msg);
   switch (errnum) {
      case LUA_OK:
         pr_err(PP_PREFIX" Lua returned success", file, line, func);
         break;
      case LUA_ERRRUN:
         pr_err(PP_PREFIX" Lua returned a runtime error",
               file, line, func);
         break;
      case LUA_ERRMEM:
         pr_err(PP_PREFIX" Lua returned a memory allocation error",
               file, line, func);
         break;
      case LUA_ERRERR:
         pr_err(PP_PREFIX" Lua returned an error"
               "while running the message handler",
               file, line, func);
         break;
      case LUA_ERRGCMM:
         pr_err(PP_PREFIX" Lua returned an error"
               "while running a __gc metamethod",
               file, line, func);
         break;
      default:
         pr_err(PP_PREFIX" Unknown error code."
               "Use this function on lua_pcall() return only.",
               file, line, func);
         break;
   }
}

void __pp_errno(int errnum, const char *msg, const char *file, const char* func, int line)
{
   pr_err(PP_PREFIX" %s", file, line, func, msg);
   switch (errnum) {
      case -ENOMEM:
         pr_err(PP_PREFIX" not enough memory", file, line, func);
         break;
      default:
         pr_err(PP_PREFIX" Unknown error code.", file, line, func);
         break;
   }
}

void __pp_warn(const char *msg, const char *file, const char* func, int line)
{
   pr_warn(PP_PREFIX" %s", file, line, func, msg);
   pr_warn("warnings should never happen and are caused because of "
         "bad configuration."
         "please refer to the README for proper configuration");
}

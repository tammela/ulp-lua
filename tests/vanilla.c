#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <linux/socket.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/tcp.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <luadata.h>

#define raise_err()  \
   ({fprintf(stderr, "%s:%s (%d)", strerror(errno), __func__, __LINE__);})

int main(void)
{
   int listener = socket(AF_INET, SOCK_STREAM, 0);
   int err;

   if (listener == -1)
      raise_err();

   struct sockaddr_in addr =
      {.sin_family = AF_INET,
      .sin_addr.s_addr = INADDR_ANY,
      .sin_port = htons(1337)};

   int on = 1;
   err = setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
   if (err == -1)
      raise_err();

   err = bind(listener, (struct sockaddr *) &addr, sizeof(addr));
   if (err == -1)
      raise_err();

   err = listen(listener, 32);
   if (err == -1)
      raise_err();

   char *msg = malloc(8192);
   if (msg == NULL) {
      raise_err();
      return -1;
   }

   lua_State *L = luaL_newstate();
   if (L == NULL) {
      raise_err();
      return -1;
   }

   luaL_openlibs(L);

   if (luaL_dofile(L, "process.lua")) {
      printf("%s\n", lua_tostring(L, -1));
      return -1;
   }

   struct sockaddr_in cl;
   socklen_t len;
   while(1) {
      int err;
      int baseref = LUA_NOREF;
      int sock = accept(listener, (struct sockaddr *) &cl, &len);
      if (sock == -1) {
         raise_err();
         return -1;
      }

      size_t msgsz = recv(sock, msg, 8192, 0);
      if (msgsz == 0)
         continue;

      if (msgsz == -1) {
         close(sock);
         raise_err();
         break;
      }

      if (lua_getglobal(L, "process") != LUA_TFUNCTION) {
         err = -EINVAL;
         raise_err();
         return -1;
      }

      baseref = ldata_newref(L, msg, msgsz);
      err = lua_pcall(L, 1, 1, 0);
      ldata_unref(L, baseref);
      if (err) {
         printf("%s\n", lua_tostring(L, -1));
         goto out;	      
      }


 out:
      close(sock);
   }

   close(listener);
   free(msg);

   return 0;
}

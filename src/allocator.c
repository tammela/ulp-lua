#include <linux/kernel.h>
#include <linux/mm.h>

#include <lua.h>

static void *vrealloc(void *ptr, size_t osize, size_t nsize)
{
   void *nptr;

   nptr = vmalloc(nsize);

   if (nptr == NULL)
      return NULL;

   memcpy(nptr, ptr, osize);
   kvfree(ptr);

   return nptr;
}

void *allocator(void *ud, void *ptr, size_t osize, size_t nsize)
{
   (void)(ud);
   void *nptr;

   if (nsize == 0) {
      kvfree(ptr);
      return NULL;
   }

   if (ptr == NULL)
      return kvmalloc(nsize, GFP_KERNEL);

   if (is_vmalloc_addr(ptr))
      return vrealloc(ptr, osize, nsize);

   nptr = krealloc(ptr, nsize, GFP_KERNEL);
   if (nptr == NULL) /* let's try a virtual reallocation */
      return vrealloc(ptr, osize, nsize);

   return nptr;
}

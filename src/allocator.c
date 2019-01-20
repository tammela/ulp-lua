#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt "\n"
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>

/*
 * check if the virtual reallocation fits the current allocation.
 * vmalloc() minimum allocation size is a page.
 * make sure the reallocation is really needed.
 */
static void *checksize(void *ptr, size_t osize, size_t nsize)
{
   if (nsize > osize) /* enlarging */
      return PAGE_ALIGN(nsize) > PAGE_ALIGN(osize) ? NULL : ptr;
   else /* shrinking or spurious */
      return ptr;
}

static void *vrealloc(void *ptr, size_t osize, size_t nsize)
{
   void *nptr;

   if (is_vmalloc_addr(ptr)) {
      nptr = checksize(ptr, osize, nsize);
      if (nptr)
         return nptr;
   }

   nptr = vmalloc(nsize);

   if (nptr == NULL)
      return NULL;

   memcpy(nptr, ptr, osize);
   kvfree(ptr);

   return nptr;
}

void *allocator(void *ud, void *ptr, size_t osize, size_t nsize)
{
   void *nptr;
   (void)(ud);

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

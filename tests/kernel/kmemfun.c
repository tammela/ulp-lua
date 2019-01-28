/**
 * A module for memory allocator testing.
 **/
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt "\n"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/slab.h>

static void do_kmalloc(void)
{
   size_t sz;
   void *ptr = NULL;
   void *optr = NULL;

   ptr = kmalloc(2, GFP_KERNEL); /* minimum allocation possible */
   for (sz = 1; ptr != NULL; sz <<= 1) {
      pr_err("=== reallocating %ld bytes ===", sz);
      optr = ptr;
      ptr = krealloc(ptr, sz, GFP_KERNEL);
      if (optr != ptr && ptr != NULL)
         pr_err("=== reallocated to new address with size %ld bytes ===",
               ksize(ptr));
      if (ptr)
         pr_err("=== real size %ld bytes ===", ksize(ptr));
      if (!ptr)
         pr_err("=== maximum reallocation is %ld ===", (sz >> 1));
   }

   /* some random allocations */
   ptr = kmalloc(100, GFP_KERNEL);
   pr_err("=== real size %ld bytes ===", ksize(ptr));
   kfree(ptr);
   ptr = kmalloc(200, GFP_KERNEL);
   pr_err("=== real size %ld bytes ===", ksize(ptr));
   kfree(ptr);
   ptr = kmalloc(300, GFP_KERNEL);
   pr_err("=== real size %ld bytes ===", ksize(ptr));
   kfree(ptr);
}

/**
 * it is known that vmalloc() only allocates whole pages.
 * more info here --> https://lwn.net/Articles/711653/
 **/

static int __init modinit(void)
{
   pr_err("=== page size %ld bytes ===", PAGE_SIZE);
   pr_err("=== kmalloc ===");
   do_kmalloc();
   return 0;
}

static void __exit modexit(void)
{
}

module_init(modinit);
module_exit(modexit);
MODULE_AUTHOR("Pedro Tammela <pctammela@gmail.com>");
MODULE_LICENSE("GPL");

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

int main(void)
{
   void *ptr = malloc(8);
   printf("minimum allocated --> %d\n", malloc_usable_size(ptr));
   ptr = malloc(64);
   printf("minimum allocated --> %d\n", malloc_usable_size(ptr));
   ptr = malloc(100);
   printf("minimum allocated --> %d\n", malloc_usable_size(ptr));
   ptr = malloc(177);
   printf("minimum allocated --> %d\n", malloc_usable_size(ptr));
   return 0;
}

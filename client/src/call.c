#include "call.h"

int allocated = 0, deallocated = 0;

void *mallocLog(size_t size)
{
    allocated++;
    void *ptr = malloc(size);
    // printf("ALLOCATED %p\n", ptr);
    return ptr;
}

void freeLog(void *ptr)
{
    deallocated++;
    // printf("DEALLOCATED %p\n", ptr);
    free(ptr);
}
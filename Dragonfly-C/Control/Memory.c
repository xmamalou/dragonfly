#include "../Internal.h"

#include <stdlib.h>

DflMemoryPool dflMemoryPoolCreate(int size)
{
    struct DflMemoryPool_T* memoryPool = malloc(sizeof(struct DflMemoryPool_T));

    if(memoryPool == NULL)
        return NULL;

    memoryPool->size = size*4; // transform from 4-byte words to bytes

    memoryPool->startingAddress = calloc(memoryPool->size, sizeof(char));
    if (memoryPool->startingAddress == NULL)
    {
        memoryPool->error = DFL_GENERIC_OOM_ERROR;
        return (DflMemoryPool)memoryPool;
    }
    memoryPool->size = 0;

    memoryPool->error = DFL_SUCCESS;

    return (DflMemoryPool)memoryPool;
}

void dflMemoryPoolExpand(int size, DflMemoryPool hMemoryPool)
{
    DFL_MEMORY_POOL->size += size*4; // transform from 4-byte words to bytes

    void* dummy = realloc(DFL_MEMORY_POOL->startingAddress, DFL_MEMORY_POOL->size);
    if (dummy == NULL)
    {
        DFL_MEMORY_POOL->error = DFL_GENERIC_OOM_ERROR;
        return;
    }
    DFL_MEMORY_POOL->startingAddress = dummy;

    DFL_MEMORY_POOL->error = DFL_SUCCESS;
}

int dflMemoryPoolSizeGet(DflMemoryPool hMemoryPool)
{
    return DFL_MEMORY_POOL->size;
}

int dflMemoryPoolErrorGet(DflMemoryPool hMemoryPool)
{
    return DFL_MEMORY_POOL->error;
}

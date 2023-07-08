/*
   Copyright 2023 Christopher-Marios Mamaloukas

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
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

void dflMemoryPoolDestroy(DflMemoryPool hMemoryPool)
{
}

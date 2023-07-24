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

/* -------------------- *
 *       INTERNAL       *
 * -------------------- */

static int _dflBlockReserveMemory(struct DflMemoryBlock_T* pMemoryBlock, bool useShared, DflDevice hDevice);
static int _dflBlockReserveMemory(struct DflMemoryBlock_T* pMemoryBlock, bool useShared, DflDevice hDevice)
{
    if (hDevice == NULL)
        pMemoryBlock->startingAddress = calloc(pMemoryBlock->size, sizeof(char)); // chars are 1 byte long. I think having the memory initialized as zeroes is a good idea.
    else
    {
        pMemoryBlock->hDevice = hDevice;

        for (int i = 0; i < ((useShared == false) ? DFL_DEVICE->localHeaps : DFL_DEVICE->nonLocalHeaps); i++)
        {
            VkMemoryAllocateInfo memInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = pMemoryBlock->size,
            .memoryTypeIndex = (useShared == false) ? DFL_DEVICE->localMem[i].heapIndex : DFL_DEVICE->nonLocalMem[i].heapIndex,
            };

            VkResult result = vkAllocateMemory(DFL_DEVICE->device, &memInfo, NULL, &pMemoryBlock->hMemoryHandle);

            if (result != VK_SUCCESS)
                continue;

            if (DFL_DEVICE->localMem[i].isHostVisible == true)
            {
                vkMapMemory(DFL_DEVICE->device, pMemoryBlock->hMemoryHandle, 0, pMemoryBlock->size, NULL, &pMemoryBlock->startingAddress);
                if (pMemoryBlock->startingAddress == NULL)
                    return DFL_VULKAN_DEVICE_UNMAPPABLE_MEMORY_ERROR;
             
            }
            else
                pMemoryBlock->miniBuff = calloc(1, sizeof(int));
            // if the memory cannot be mapped, we will create a command buffer that will
            // copy the data from the following 4-byte buffer to the buffer that is on the 
            // device's memory when the user attempts to put data in some allocated memory.
            // Since the buffer's size will be known at allocation time, both it and the 
            // command buffer will be created at `dflAllocate`.
        }
    }

    if ((pMemoryBlock->startingAddress == NULL) && (pMemoryBlock->hMemoryHandle == NULL))
        return DFL_GENERIC_OOM_ERROR; // if hMemoryHandle isn't NULL, it's not unlikely that the memory just isn't mapped (which would make the startingAddress NULL). But that's not a problem, necessarily.

    if ((hDevice != NULL) && (pMemoryBlock->hMemoryHandle == NULL))
        return DFL_VULKAN_DEVICE_NO_MEMORY_ERROR;

    return DFL_SUCCESS;
}


/* -------------------- *
 *   INITIALIZE         *
 * -------------------- */

DflMemoryPool dflMemoryPoolCreate(int size, bool useShared, DflDevice hDevice)
{
    struct DflMemoryPool_T* memoryPool = calloc(1, sizeof(struct DflMemoryPool_T));

    if(memoryPool == NULL)
        return NULL;

    memoryPool->size = size*4; // transform from 4-byte words to bytes
    memoryPool->used = 0;
    memoryPool->error = DFL_SUCCESS;

    struct DflMemoryBlock_T* block = calloc(1, sizeof(struct DflMemoryBlock_T));
    if (block == NULL)
    {
        memoryPool->error = DFL_CORE_MEMORY_UNAVAILABLE_BLOCKS_ERROR;
        return (DflMemoryPool)memoryPool;
    }

    block->size = size*4; // transform from 4-byte words to bytes
    block->used = 0;
    block->next = NULL;
    block->firstAvailableSlot = 0;


    memoryPool->error = _dflBlockReserveMemory(block, useShared, hDevice);

    memoryPool->firstBlock = block;

    return (DflMemoryPool)memoryPool;
}

/* -------------------- *
 *   CHANGE             *
 * -------------------- */

void dflMemoryPoolExpand(int size, DflMemoryPool hMemoryPool, bool useShared, DflDevice hDevice)
{
    DFL_MEMORY_POOL->size += size * 4; // transform from 4-byte words to bytes

    struct DflMemoryBlock_T* block = DFL_MEMORY_POOL->firstBlock;
    while (block->next != NULL)
    {
        block = block->next;
        continue;
    }

    struct DflMemoryBlock_T* newBlock = calloc(1, sizeof(struct DflMemoryBlock_T));
    if (newBlock == NULL)
    {
        DFL_MEMORY_POOL->error = DFL_CORE_MEMORY_UNAVAILABLE_BLOCKS_ERROR;
        return;
    }

    newBlock->size = size * 4; // transform from 4-byte words to bytes
    newBlock->used = 0;
    newBlock->next = NULL;
    newBlock->firstAvailableSlot = 0;

    DFL_MEMORY_POOL->error = _dflBlockReserveMemory(newBlock, useShared, hDevice);

    block->next = newBlock;
}

DflBuffer dflAllocate(int size, DflMemoryPool hMemoryPool)
{
    // allocation happens this way: Dragonfly checks if a memory block has available space. If it has, it allocates a buffer chunk and reserves the space for it.
    // The final buffer will consist of such chunks linked together. If a block has no available space, Dragonfly will check if there is another block. If there is,
    // it will allocate a new chunk there. If there isn't, it will throw an OOM error.
    if (DFL_MEMORY_POOL->used >= DFL_MEMORY_POOL->size)
    {
        DFL_MEMORY_POOL->error = DFL_CORE_MEMORY_OOM_ERROR;
        return NULL;
    }

    struct DflBuffer_T* buffer = calloc(1, sizeof(struct DflBuffer_T));
    if (buffer == NULL)
    {
        DFL_MEMORY_POOL->error = DFL_GENERIC_OOM_ERROR;
        return NULL;
    }

    // every buffer will have at least one chunk
    struct DflBufferChunk_T* chunk = calloc(1, sizeof(struct DflBufferChunk_T));
    if (chunk == NULL)
    {
        DFL_MEMORY_POOL->error = DFL_GENERIC_OOM_ERROR;
        return NULL;
    }
    buffer->firstChunk = chunk;

    struct DflBufferChunk_T* nextChunk = NULL;
    chunk->next = nextChunk;

    struct DflMemoryBlock_T* block = DFL_MEMORY_POOL->firstBlock;
    struct DflMemoryBlock_T* nextBlock = block->next;

    while (block != NULL)
    {
        if (block->used >= block->size)
        {
            block = nextBlock;
            nextBlock = block->next;
            continue;
        }

        // the buffer may need to be split between blocks, however it is possible that a block may not have enough space
        // for that reason, we add up the size to the buffer's metadata *after* the space has been reserved. If, afterwards
        // the size reported on the metadata is less than the size requested, we know that there is not enough memory for the 
        // buffer.

        if ((block->used + (size * 4)) > block->size) // this means that the buffer cannot have only one chunk
        {
            nextChunk = calloc(1, sizeof(struct DflBufferChunk_T));
            if (nextChunk == NULL)
            {
                DFL_MEMORY_POOL->error = DFL_GENERIC_OOM_ERROR;
                return NULL;
            }
        }


    }

    DFL_MEMORY_POOL->used += size * 4;

    return (DflBuffer)buffer;
}

/* -------------------- *
 *   GET & SET          *
 * -------------------- */

int dflMemoryPoolSizeGet(DflMemoryPool hMemoryPool)
{
    return DFL_MEMORY_POOL->size;
}

inline int dflMemoryPoolBlockCountGet(DflMemoryPool hMemoryPool)
{
    int blockCount = 1; // every memory pool has at least one block

    struct DflMemoryBlock_T* block = DFL_MEMORY_POOL->firstBlock;

    while (block->next != NULL)
    {
        blockCount++;
        block = block->next;
    }

    return blockCount;
}

int dflMemoryPoolErrorGet(DflMemoryPool hMemoryPool)
{
    return DFL_MEMORY_POOL->error;
}

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

void dflMemoryPoolDestroy(DflMemoryPool hMemoryPool)
{
    struct DflMemoryBlock_T* block = DFL_MEMORY_POOL->firstBlock;
    struct DflMemoryBlock_T* nextBlock = NULL;
    while (block != NULL)
    {
        nextBlock = block->next;
        if (block->hMemoryHandle == NULL)
            free(block->startingAddress);
        else
        {
            if (block->startingAddress != NULL)
                vkUnmapMemory(((struct DflDevice_T*)block->hDevice)->device, block->hMemoryHandle);

            if (block->miniBuff != NULL)
                free(block->miniBuff);

            vkFreeMemory(((struct DflDevice_T*)block->hDevice)->device, block->hMemoryHandle, NULL);
        }
        free(block);
        block = nextBlock;
    }
    free(DFL_MEMORY_POOL);
}

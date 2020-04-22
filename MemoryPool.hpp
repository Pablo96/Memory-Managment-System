#pragma once
#include <cstddef>
#include <cassert>
#include <memory>
#include <algorithm>
#include <iostream>

#define NewAndDeleteOperators(Class)\
inline void* operator new(size_t size)\
{ return g_memory->Alloc(size); }\
inline void operator delete(void* ptr)\
{ g_memory->Free(ptr); }


typedef unsigned char byte;
typedef unsigned long long uint64;

struct MemBlock             // size 40 bytes 
{
    uint64      size;       // size of the free memory slot (bytes)
    uint64      usedSize;       // size of the used memory in the slot (bytes)
    MemBlock    *next;      // points to a FreeSlot Structure
    byte        *data;      // pointer to the Data
    bool        isAlloc;    // true, when this MemoryChunks points to a "Data"-Block
                            // which can be deallocated via "free()"
};

class MemorySystem
{
    MemBlock        *firstBlock;
    MemBlock        *lastBlock;
    MemBlock        *cursor;

    const uint64    blockSize;
    const uint64    minBlockSize;

    uint64          totalPoolSize;
    uint64          usedPoolSize;
    uint64          freePoolSize;
    
    uint64          blockCount;
    uint64          allocCount;

    bool            should_expand;
    bool            defrag_enabled;

public:
    /* Pool size in MemorySystem::MEM_UNIT*/
    MemorySystem(const uint64 in_initialPoolSize, const uint64 in_blockSize, const uint64 in_minAllocSize,
        const bool in_shouldExpand = false, const bool in_defragEnabled = false);
    ~MemorySystem();

    void*   Alloc(uint64 in_size) { return GetMemory(in_size); }

    void    Free(void* pointer) { FreeMemory(pointer); }

private:
    //return a block which can hold the requested amount of memory, or NULL, if none was found.
    MemBlock*   FindChunkSuitableToHoldMemory(const uint64 sMemorySize);
    MemBlock*   FindChunkHoldingPointerTo(void* ptrMemoryBlock);
    MemBlock*   SkipChunks(MemBlock* ptrStartChunk, const uint64 uiChunksToSkip);
    MemBlock*   SetChunkDefaults(MemBlock* ptrChunk);

    uint64 CalculateNeededChunks(const uint64 sMemorySize);
    uint64 CalculateBestMemoryBlockSize(const uint64 allocSize);

    bool AllocateMemory(const uint64 sMemorySize);
    bool LinkChunksToData(MemBlock* ptrNewChunks, const uint64 uiChunkCount, byte* ptrNewMemBlock);
    bool RecalcChunkMemorySize(MemBlock* ptrChunk, const uint64 uiChunkCount);

    void* GetMemory(const uint64 sMemorySize);
    void FreeMemory(void* ptrMemoryBlock);
    void FreeChunks(MemBlock* ptrChunk);
    inline void SetMemoryChunkValues(MemBlock* ptrChunk, const uint64 sMemBlockSize);
};



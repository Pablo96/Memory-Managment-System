#include "MemoryPool.hpp"
#include <iostream>
#include <chrono>

MemorySystem::MemorySystem(const uint64 in_initialPoolSize, const uint64 in_blockSize, const uint64 in_minAllocSize,
    const bool in_shouldExpand, const bool in_defragEnabled) :
    blockSize(in_blockSize), minBlockSize(in_minAllocSize),
    firstBlock(nullptr), lastBlock(nullptr), cursor(nullptr),
    totalPoolSize(0L), usedPoolSize(0L), freePoolSize(0L),
    blockCount(0L), allocCount(0L)
{
    AllocateMemory(in_initialPoolSize);
}

MemorySystem::~MemorySystem()
{
    MemBlock* ptrChunk = firstBlock;
    while (ptrChunk)
    {
        if (ptrChunk->isAlloc)
        {
            free(((void*)(ptrChunk->data)));
        }
        ptrChunk = ptrChunk->next;
    }

    ptrChunk = firstBlock;
    MemBlock* ptrChunkToDelete = nullptr;
    while (ptrChunk)
    {
        if (ptrChunk->isAlloc)
        {
            if (ptrChunkToDelete)
            {
                free(((void*)ptrChunkToDelete));
            }
            ptrChunkToDelete = ptrChunk;
        }
        ptrChunk = ptrChunk->next;
    }

    // Check for possible Memory-Leaks...
    assert((allocCount == 0) && "WARNING : Memory-Leak : You have not freed all allocated Memory");
}

MemBlock* MemorySystem::FindChunkSuitableToHoldMemory(const uint64 sMemorySize)
{
    // Find a Chunk to hold *at least* "sMemorySize" Bytes.
    uint64 uiChunksToSkip = 0L;
    bool bContinueSearch = true;
    MemBlock* ptrChunk = cursor; // Start search at Cursor-Pos.
    for (unsigned int i = 0; i < blockCount; i++)
    {
        if (ptrChunk)
        {
            if (ptrChunk == lastBlock) // End of List reached : Start over from the beginning
            {
                ptrChunk = firstBlock;
            }

            if (ptrChunk->size >= sMemorySize)
            {
                if (ptrChunk->usedSize == 0)
                {
                    cursor = ptrChunk;
                    return ptrChunk;
                }
            }
            uiChunksToSkip = CalculateNeededChunks(ptrChunk->usedSize);
            if (uiChunksToSkip == 0) uiChunksToSkip = 1;
            ptrChunk = SkipChunks(ptrChunk, uiChunksToSkip);
        }
        else
        {
            bContinueSearch = false;
        }
    }
    return nullptr;
}

inline void MemorySystem::SetMemoryChunkValues(MemBlock* in_ptrChunk, const uint64 in_blockSize)
{
    if (in_ptrChunk) // && (ptrChunk != m_ptrLastChunk))
        in_ptrChunk->usedSize = in_blockSize;
    else
        assert(false && "Error : Invalid NULL-Pointer passed");
}

void* MemorySystem::GetMemory(const uint64 sMemorySize)
{
    uint64 sBestMemBlockSize = CalculateBestMemoryBlockSize(sMemorySize);
    MemBlock* ptrChunk = nullptr;
    while (!ptrChunk)
    {
        // Is a Chunks available to hold the requested amount of Memory ?
        ptrChunk = FindChunkSuitableToHoldMemory(sBestMemBlockSize);
        if (!ptrChunk)
        {
            // No chunk can be found
            // => Memory-Pool is to small. We have to request 
            //    more Memory from the Operating-System....
            sBestMemBlockSize = std::max(sBestMemBlockSize, CalculateBestMemoryBlockSize(minBlockSize));
            AllocateMemory(sBestMemBlockSize);
        }
    }

    // Finally, a suitable Chunk was found.
    // Adjust the Values of the internal "TotalSize"/"UsedSize" Members and 
    // the Values of the MemoryChunk itself.
    usedPoolSize += sBestMemBlockSize;
    freePoolSize -= sBestMemBlockSize;
    allocCount++;
    SetMemoryChunkValues(ptrChunk, sBestMemBlockSize);

    // eventually, return the Pointer to the User
    return ((void*)ptrChunk->data);
}

void MemorySystem::FreeMemory(void* ptrMemoryBlock)
{
    // Search all Chunks for the one holding the "ptrMemoryBlock"-Pointer
 // ("SMemoryChunk->Data == ptrMemoryBlock"). Eventually, free that Chunks,
 // so it beecomes available to the Memory-Pool again...
    MemBlock* ptrChunk = FindChunkHoldingPointerTo(ptrMemoryBlock);
    if (ptrChunk)
    {
        //std::cerr << "Freed Chunks OK (Used memPool Size : " << m_sUsedMemoryPoolSize << ")" << std::endl ;
        FreeChunks(ptrChunk);
    }
    else
    {
        assert(false && "ERROR : Requested Pointer not in Memory Pool");
    }
    assert((allocCount > 0) && "ERROR : Request to delete more Memory then allocated.");
    allocCount--;
}

void MemorySystem::FreeChunks(MemBlock* ptrChunk)
{
    // Make the Used Memory of the given Chunk available
  // to the Memory Pool again.

    MemBlock* ptrCurrentChunk = ptrChunk;
    uint64 uiChunkCount = CalculateNeededChunks(ptrCurrentChunk->usedSize);
    for (unsigned int i = 0; i < uiChunkCount; i++)
    {
        if (ptrCurrentChunk)
        {
#ifdef _DEBUG
            // Step 1 : Set the allocated Memory to 'FREEED_MEMORY_CONTENT'
            // Note : This is fully Optional, but usefull for debugging
            memset((void*)ptrCurrentChunk->data, 0xDD, blockSize);
#endif
            // Step 2 : Set the Used-Size to Zero
            ptrCurrentChunk->usedSize = 0;

            // Step 3 : Adjust Memory-Pool Values and goto next Chunk
            usedPoolSize -= blockSize;
            ptrCurrentChunk = ptrCurrentChunk->next;
        }
    }
}

MemBlock* MemorySystem::FindChunkHoldingPointerTo(void* ptrMemoryBlock)
{
    MemBlock* ptrTempChunk = firstBlock;
    while (ptrTempChunk)
    {
        if (ptrTempChunk->data == (byte*)ptrMemoryBlock)
        {
            break;
        }
        ptrTempChunk = ptrTempChunk->next;
    }
    return ptrTempChunk;
}

MemBlock* MemorySystem::SkipChunks(MemBlock* ptrStartChunk, const uint64 uiChunksToSkip)
{
    MemBlock* ptrCurrentChunk = ptrStartChunk;
    for (unsigned int i = 0; i < uiChunksToSkip; i++)
    {
        if (ptrCurrentChunk)
        {
            ptrCurrentChunk = ptrCurrentChunk->next;
        }
        else
        {
            // Will occur, if you try to Skip more Chunks than actually available
            // from your "ptrStartChunk" 
            assert(false && "Error : Chunk == NULL was not expected.");
            break;
        }
    }
    return ptrCurrentChunk;
}

uint64 MemorySystem::CalculateNeededChunks(const uint64 sMemorySize)
{
    float f = (float)sMemorySize / (float)blockSize;
    return (uint64) ceil(f);
}

uint64 MemorySystem::CalculateBestMemoryBlockSize(const uint64 allocSize)
{
    return CalculateNeededChunks(allocSize) * blockSize;
}

bool MemorySystem::AllocateMemory(const uint64 sMemorySize)
{
    uint64 uiNeededChunks = CalculateNeededChunks(sMemorySize);
    uint64 sBestMemBlockSize = CalculateBestMemoryBlockSize(sMemorySize);

    byte* ptrNewMemBlock = (byte*) malloc(sBestMemBlockSize);
    MemBlock* ptrNewChunks = (MemBlock*)malloc(uiNeededChunks * sizeof(MemBlock));
    assert(((ptrNewMemBlock) && (ptrNewChunks)) && "Error : System ran out of Memory");

    // Adjust internal Values (Total/Free Memory, etc.)
    totalPoolSize += sBestMemBlockSize;
    freePoolSize+= sBestMemBlockSize;
    blockCount += uiNeededChunks;

    // Usefull for Debugging : Set the Memory-Content to a defined Value
#ifdef _DEBUG
    memset(((void*)ptrNewMemBlock), 0xFF, sBestMemBlockSize);
#endif
    // Associate the allocated Memory-Block with the Linked-List of MemoryChunks
    return LinkChunksToData(ptrNewChunks, uiNeededChunks, ptrNewMemBlock);
}

bool MemorySystem::LinkChunksToData(MemBlock* ptrNewChunks, const uint64 uiChunkCount, byte* ptrNewMemBlock)
{
    MemBlock* ptrNewChunk = nullptr;
    uint64 uiMemOffSet = 0;

    bool bAllocationChunkAssigned = false;

    for (uint64 i = 0; i < uiChunkCount; i++)
    {
        if (!firstBlock)
        {
            firstBlock = SetChunkDefaults(&ptrNewChunks[0]);
            lastBlock = firstBlock;
            cursor = firstBlock;
        }
        else
        {
            ptrNewChunk = SetChunkDefaults(&ptrNewChunks[i]);
            lastBlock->next = ptrNewChunk;
            lastBlock = ptrNewChunk;
        }

        uiMemOffSet = i * (uint64)blockSize;
        lastBlock->data = &(ptrNewMemBlock[uiMemOffSet]);

        // The first Chunk assigned to the new Memory-Block will be 
        // a "AllocationChunk". This means, this Chunks stores the
        // "original" Pointer to the MemBlock and is responsible for
        // "free()"ing the Memory later....
        if (!bAllocationChunkAssigned)
        {
            lastBlock->isAlloc = true;
            bAllocationChunkAssigned = true;
        }
    }
    return RecalcChunkMemorySize(firstBlock, blockCount);
}

bool MemorySystem::RecalcChunkMemorySize(MemBlock* ptrChunk, const uint64 uiChunkCount)
{
    uint64 uiMemOffSet = 0;
    for (uint64 i = 0; i < uiChunkCount; i++)
    {
        if (ptrChunk)
        {
            uiMemOffSet = i * blockSize;
            ptrChunk->size = totalPoolSize - uiMemOffSet;
            ptrChunk = ptrChunk->next;
        }
        else
        {
            assert(false && "Error : ptrChunk == NULL");
            return false;
        }
    }
    return true;
}

MemBlock* MemorySystem::SetChunkDefaults(MemBlock* ptrChunk)
{
    if (ptrChunk)
    {
        ptrChunk->data = nullptr;
        ptrChunk->size = 0L;
        ptrChunk->usedSize = 0L;
        ptrChunk->isAlloc = false;
        ptrChunk->next = nullptr;
    }
    return ptrChunk;
}
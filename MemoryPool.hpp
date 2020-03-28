#pragma once
#include <memory>

#define NewAndDeleteOperators(Class)\
inline void* operator new(size_t size)\
{ return g_memory->Alloc(size); }\
inline void operator delete(void* ptr)\
{ g_memory->Free(ptr); }

class MemorySystem
{
    static const size_t MEM_UNIT;

    struct MemSlot
    {
        /* size of the free slot in bytes */
        size_t  size;
        /* points to a FreeSlot Structure */
        void    *next;
        /* &data   points to it self */
        void    *data;
    };
    size_t      pool_size;
    size_t      pool_size_bytes;
    bool        should_expand;
    bool        defrag_enabled;
    // List of freed elements.
    MemSlot     *last_slot;

    // Whole chunk of memory requested at startup
    void        *memory_pool;

public:
    /* Pool size in MemorySystem::MEM_UNIT*/
    MemorySystem(const size_t in_poolSize, const bool in_shouldExpand = false, const bool in_defragEnabled = false)
    : pool_size(in_poolSize), pool_size_bytes(pool_size* (MEM_UNIT + 20)), should_expand(in_shouldExpand),
        defrag_enabled(in_defragEnabled), memory_pool(malloc(pool_size_bytes)), last_slot(nullptr)
    {
        if (memory_pool)
        {
            /* debug */
            memset(memory_pool, 0, pool_size_bytes);
            /* set whole memory as the available slot */
            last_slot = (MemSlot*) memory_pool;
            last_slot->size = in_poolSize * MEM_UNIT;
            last_slot->next = nullptr;
            last_slot->data = &last_slot->data;
        }
    }
    
    void*   Alloc(size_t in_size);

    void    Free(void* pointer);
};

MemorySystem* g_memory;

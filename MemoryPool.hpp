#pragma once
#include <memory>

#define NewAndDeleteOperators(Class)\
inline void* operator new(size_t size)\
{ return g_memory->Alloc(size); }\
inline void operator delete(void* ptr)\
{ g_memory->Free(ptr); }

class MemorySystem
{
    struct MemSlot
    {
        /* size of the free slot in bytes */
        size_t  size;
        /* points to a FreeSlot Structure */
        void    *next;
        /* &data   points to it self */
        void    *data;
    };
    const size_t    min_alloc_size;
    size_t          pool_size_bytes;
    MemSlot         *last_slot;     // List of freed elements.
    void            *memory_pool;   // Whole chunk of memory requested at startup
    bool            should_expand;
    bool            defrag_enabled;

public:
    /* Pool size in MemorySystem::MEM_UNIT*/
    MemorySystem(const size_t in_minAllocSize, const size_t in_maxMinSizeAllocs, const bool in_shouldExpand = false, const bool in_defragEnabled = false)
    :   min_alloc_size((in_minAllocSize >= 8) ? in_minAllocSize : 8),   // minimo es 8 dado que el puntero tiene 8 bytes y debe ser tomado en cuenta
        pool_size_bytes((min_alloc_size + sizeof(size_t) * 2)* in_maxMinSizeAllocs),
        should_expand(in_shouldExpand), defrag_enabled(in_defragEnabled), memory_pool(nullptr), last_slot(nullptr)
    {
        memory_pool = malloc(pool_size_bytes);
        if (memory_pool)
        {
#ifdef _DEBUG
            memset(memory_pool, 0, pool_size_bytes);
#endif
            /* set whole memory as the available slot */
            last_slot = (MemSlot*) memory_pool;
            last_slot->size = pool_size_bytes;
            last_slot->next = nullptr;
            last_slot->data = nullptr;
        }
    }
    
    void*   Alloc(size_t in_size);

    void    Free(void* pointer);
};



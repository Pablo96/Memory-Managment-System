#include "MemoryPool.hpp"
#include <iostream>
#include <chrono>

void* MemorySystem::Alloc(size_t in_size)
{
    /*  If pool is full */
    if (!last_slot)
    {
        /* and we can't expand we have a big problem! */
        if (!should_expand)
        {
mem_alloc_except:
            std::cout << "Mem Alloc Error: No se pudo alojar " << in_size << " bytes!" << std::endl;
            throw nullptr;
        }
        else
        {
            // Expand pool by pool_size
        }
    }

    /* Calculate the requested size in mem units multiple */
    size_t requested_size = (in_size < min_alloc_size) ? min_alloc_size : in_size;
  
    /* Search for free slot for the requested size */
    MemSlot  **slot_loc = &last_slot;
    MemSlot   *slot = nullptr;
    while ( (*slot_loc) != nullptr)
    {
        if ((*slot_loc)->size >= requested_size)
        {
            slot = *slot_loc;
            break;
        }
        slot_loc = (MemSlot**)&((*slot_loc)->next);
    }

    /* if not slot found print an error */
    if (slot == nullptr)
        goto mem_alloc_except;

    /* reduce the block if it is N*MEMUNIT bigger than the requested size (minimum of 1*MEM_UNIT) */
    if (slot->size > requested_size)
    {
        size_t dif = slot->size - requested_size;
        auto free_slot = (*slot_loc) = (MemSlot*)((char*)(&slot->data) + requested_size + 4);
        free_slot->size = dif;
        free_slot->next = slot->next;
    }
    /* Else we extract the free slot from the list */
    else
    {
        *slot_loc = (MemSlot*)slot->next;
    }

    slot->size = requested_size;
    /* We can avoid setting next to 0 since it will be assigned in free */
    slot->next = nullptr;
    return &slot->data;
}

/* Just add it to the free list and that's all */
void MemorySystem::Free(void* pointer)
{
    MemSlot* mem_ptr   = (MemSlot*)((char*)pointer - 16);
    size_t point       = (size_t)mem_ptr;
    size_t mem_ptr_end = (size_t)((char*)&mem_ptr->data + 8);

    if (defrag_enabled)
    {
        MemSlot** slot_loc = nullptr;
        bool     merged = false;
        for (slot_loc = &last_slot; *slot_loc != nullptr; slot_loc = (MemSlot**)&(*slot_loc)->next)
        {
            MemSlot* slot = *slot_loc;
            size_t range_start  = (size_t)slot;
            size_t range_end    = (size_t)((char*)slot + slot->size + 16);

            /* if pointer is directly up becomes the free slot
             * and we merge the slot into it */
            if (mem_ptr_end == range_start)
            {
                auto free_slot = *slot_loc = mem_ptr;
                size_t size_2Add = (range_end - (size_t)&mem_ptr->data);
                free_slot->size = slot->size + size_2Add;
                free_slot->next = slot->next;
                merged = true;
                break;
            }
            /* if pointer is directly down then we merge it into the slot */
            else if (point == range_end)
            {
                slot->size += ((size_t)pointer - (size_t)((char*)&mem_ptr->data + mem_ptr->size));
                merged = true;
                break;
            }
        }
        if (!merged)
            goto add_FreeSlot;
    }
    else
    {
add_FreeSlot:
        /* We should not touch the size nor the data here only the next field.
         * IMPORTANT: Remember that the pointer points to MemSlot->data so we need
         * to go back 16 bytes. */
        MemSlot* next = last_slot;
        last_slot = mem_ptr;
        last_slot->next = (void*)next;
    }
}

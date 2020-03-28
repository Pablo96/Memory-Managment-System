#include "MemoryPool.hpp"
#include <iostream>
#include <chrono>


#define PERFORMANCE_TEST

#define TRACE_METHOD() std::cout << this << " " << __FUNCTION__ << " number: " << x << "\n";

const size_t MemorySystem::MEM_UNIT = 4; // bytes

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
      throw 0xAAA;
    }
    else
    {
      // Expand pool by pool_size
    }
  }

  /* Calculate the requested size in mem units multiple */
  size_t padding_size = (in_size < MEM_UNIT) ? MEM_UNIT - in_size : in_size % MEM_UNIT;
  size_t requested_size = in_size + padding_size;
  
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
  if (slot->size - MEM_UNIT > requested_size)
  {
    size_t dif = slot->size - requested_size;
    auto free_slot = (*slot_loc) = (MemSlot*)((char*)(slot->data) + requested_size + 4);
    free_slot->size = dif;
    free_slot->next = slot->next;
    free_slot->data = &free_slot->data;
  }
  /* Else we extract the free slot from the list */
  else
  {
    *slot_loc = (MemSlot*)slot->next;
  }

  slot->size = requested_size;
  /* We can avoid setting next to 0 since it will be assigned in free */
  slot->next = nullptr;
  slot->data = &slot->data;
  return slot->data;
}

/* Just add it to the free list and that's all */
void MemorySystem::Free(void* pointer)
{
    pointer = (MemSlot*)((char*)pointer - 16);
    MemSlot* slot = nullptr;
    if (defrag_enabled)
    {
        for (slot = last_slot; slot != nullptr; slot = (MemSlot*) slot->next)
        {
            size_t range_start  = (size_t)slot;
            size_t range_end    = (size_t)((char*)slot + slot->size + 16);
            size_t point        = (size_t)pointer;

            /* if pointer is directly up or down of this slot */
            if (point < range_start && point > range_end)
            {

            }
        }
    }
    else
    {
        /* We should not touch the size nor the data here only the next field.
         * IMPORTANT: Remember that the pointer points to MemSlot->data so we need
         * to go back 16 bytes. */
        MemSlot* next = last_slot;
        last_slot = (MemSlot*)pointer;
        last_slot->next = (void*)next;
    }
}

struct Foo {
  int x = 42;
#ifndef PERFORMANCE_TEST
  Foo() {
      TRACE_METHOD();
  }
  Foo(int x) : x(x) {
      TRACE_METHOD();
  }
  ~Foo() {
      TRACE_METHOD();
  };
#else
  Foo() {}
  Foo(int x) : x(x) {}
  ~Foo() = default;
#endif
  NewAndDeleteOperators(Foo)
};

struct FooSysCall {
    int x = 42;
    FooSysCall() {}
    FooSysCall(int x) : x(x) {}
    ~FooSysCall() = default;
};

struct FooObject
{
    int     x;
    char    c;
    size_t  s;

    FooObject()
        : x(32), c('F'), s((size_t)&x) {
        TRACE_METHOD();
    }
    FooObject(const size_t in_s, const int in_x, const char in_c)
        : x(in_x), c(in_c), s(in_s) {
        TRACE_METHOD();
    }
    ~FooObject() { TRACE_METHOD(); }

    NewAndDeleteOperators(FooObject)
};

int FirstTestDelAlloc();
int PerformanceNoMemSys();
int PerformanceMemSys();
int main(int argc, char *argv[])
{
#ifndef PERFORMANCE_TEST
    return FirstTestDelAlloc();
#else
    bool edge_case = true;
    g_memory = edge_case ? new MemorySystem(5) : new MemorySystem(1);

    const size_t tests_count = 10;
    double improvement_media = 0;
    for (int i = 0; i < tests_count; i++)
    {
        long long time_CallSys;
        long long time_MemSys;
        std::cout << "Test: " << i + 1 << ":\n";
        {
            auto start = std::chrono::steady_clock::now();
            PerformanceNoMemSys();
            auto end = std::chrono::steady_clock::now();
            auto time = end - start;
            time_CallSys = std::chrono::duration_cast<std::chrono::milliseconds>(time).count();
            std::cout << "\tTime sys call: " << time_CallSys << "ms\n";
        }
        {
            auto start = std::chrono::steady_clock::now();
            PerformanceMemSys();
            auto end = std::chrono::steady_clock::now();
            auto time = end - start;
            time_MemSys = std::chrono::duration_cast<std::chrono::milliseconds>(time).count();
            std::cout << "\tTime mem sys: " << time_MemSys << "ms\n";
        }
        double improvement = 1.0 - (double) time_MemSys / time_CallSys;
        improvement_media += improvement;
        std::cout << "\tMemSys improvement: " << improvement * 100 << "%\n";
    }
    // flush
    std::cout << "media: " << improvement_media * 100.0 / tests_count << "%" << std::endl;
    return 0;
#endif
}

int FirstTestDelAlloc()
{
    g_memory = new MemorySystem(5);

    Foo* p1 = nullptr, * p2 = nullptr, * p3 = nullptr;

    p1 = new Foo();
    p2 = new Foo(44);
    p3 = new Foo(48);

    std::cout << "First allocs:\n";
    std::cout << "p1->x=" << p1->x << "\n";
    std::cout << "p2->x=" << p2->x << "\n";
    std::cout << "p3->x=" << p3->x << "\n";

    delete p2;
    std::cout << "P2 freed (pool fragmentation):\n";
    std::cout << "p1->x=" << p1->x << "\n";
    std::cout << "p2->x=" << p2->x << "\n";
    std::cout << "p3->x=" << p3->x << "\n";

    p2 = new Foo(90);
    std::cout << "After fragmentation:\n";
    std::cout << "p1->x=" << p1->x << "\n";
    std::cout << "p2->x=" << p2->x << "\n";
    std::cout << "p3->x=" << p3->x << "\n";

    delete p1;
    delete p3;
    delete p2;
    std::cout << "All freed:\n";
    std::cout << "p1->x=" << p1->x << "\n";
    std::cout << "p2->x=" << p2->x << "\n";
    std::cout << "p3->x=" << p3->x << "\n";


    p1 = new Foo();
    auto po = new FooObject(0xFEA951, 16,'G');

    std::cout << "FooObject in:\n";
    std::cout << "p1->x=" << p1->x << "\n";
    std::cout << "po->x=" << po->x << "\n";

    delete p1;
    delete po;

    return 0;
}

int PerformanceNoMemSys()
{
    for (int j = 0; j < 5000; j++)
    {
        for (int i = 0; i < 1000; i++)
        {
            auto foo = new FooSysCall(i);
            delete foo;
        }
    }
    return 0;
}
int PerformanceMemSys()
{
    for (int j = 0; j < 5000; j++)
    {
        for (int i = 0; i < 1000; i++)
        {
            auto foo = new Foo(i);
            delete foo;
        }
    }
    return 0;
}
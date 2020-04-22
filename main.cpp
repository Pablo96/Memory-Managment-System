#include "MemoryPool.hpp"
#include <iostream>

//#define PERFORMANCE_TEST
#define TRACE_METHOD() std::cout << this << " " << __FUNCTION__ << " number: " << x << "\n";

typedef long long type_;
MemorySystem* g_memory = nullptr;

struct Foo {
    type_ x = 42;
#ifndef PERFORMANCE_TEST
    Foo() {
        TRACE_METHOD();
    }
    Foo(type_ x) : x(x) {
        TRACE_METHOD();
    }
    ~Foo() {
        TRACE_METHOD();
    };
#else
    Foo() {}
    Foo(type_ x) : x(x) {}
    ~Foo() = default;
#endif
    NewAndDeleteOperators(Foo)
};

struct FooSysCall {
    type_ x = 42;
    FooSysCall() {}
    FooSysCall(type_ x) : x(x) {}
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
int main(int argc, char* argv[])
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
        double improvement = 1.0 - (double)time_MemSys / time_CallSys;
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
try {
    g_memory = new MemorySystem(4, 5, false, true);

    Foo* p1 = nullptr, * p2 = nullptr, * p3 = nullptr;

    p1 = new Foo();
    p2 = new Foo(44);
    p3 = new Foo(48);

    std::cout << "First allocs:\n";
    std::cout << "p1->x=" << p1->x << "\n";
    std::cout << "p2->x=" << p2->x << "\n";
    std::cout << "p3->x=" << p3->x << "\n";

    delete p2;
    p2 = nullptr;
    std::cout << "P2 freed (pool fragmentation):\n";
    std::cout << "p1->x=" << p1->x << "\n";
    std::cout << "p3->x=" << p3->x << "\n";

    p2 = new Foo(90);
    std::cout << "After fragmentation:\n";
    std::cout << "p1->x=" << p1->x << "\n";
    std::cout << "p2->x=" << p2->x << "\n";
    std::cout << "p3->x=" << p3->x << "\n";

    /* order is important for the test*/
    delete p1; p1 = nullptr;
    delete p3; p3 = nullptr;
    delete p2; p2 = nullptr;

    std::cout << "\nAll freed, defragmented if enabled \n\n";

    p1 = new Foo();

    std::cout << "FooObject ("<< sizeof(FooObject) << ") in:\n";
    auto po = new FooObject(0xFEA951, 16, 'G');
    std::cout << "p1->x=" << p1->x << "\n";
    std::cout << "po->x=" << po->x << "\n";

    delete p1;
    delete po;
} catch (...) {}
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

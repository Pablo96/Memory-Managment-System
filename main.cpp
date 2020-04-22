#include "MemoryPool.hpp"
#include <iostream>

//#define PERFORMANCE_TEST
#define TRACE_METHOD() std::cout << this << " " << __FUNCTION__ << " number: " << x << "\n";

typedef int type_;
MemorySystem* g_memory = nullptr;

struct Foo {
    type_ x = 42;
    Foo() {
        TRACE_METHOD();
    }
    Foo(type_ x) : x(x) {
        TRACE_METHOD();
    }
    ~Foo() {
        TRACE_METHOD();
    };
    NewAndDeleteOperators(Foo)
};


int FirstTestDelAlloc();

int main(int argc, char* argv[])
{
    return FirstTestDelAlloc();
}

int FirstTestDelAlloc()
{
    g_memory = new MemorySystem(128L, 1L, 2L);

    auto p0 = new Foo();
    auto p1 = new Foo(214);
    auto p2 = new Foo(15);

    return 0;
}
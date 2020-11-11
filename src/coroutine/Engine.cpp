#include <afina/coroutine/Engine.h>

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#include <cassert>

namespace Afina {
namespace Coroutine {

void Engine::Store(context &ctx) {
    set_after(ctx); // higher on stack

    assert (ctx.Hight - ctx.Low >= 0);
    printf ("\n\nLow = %p\n\n", ctx.Low);
    printf ("\n\nHight = %p\n\n", ctx.Hight);
    printf ("\n\nSIZE = %d\n\n", ctx.Hight - ctx.Low);
    std::size_t memory_need = ctx.Hight - ctx.Low;

    printf("Enter heap alloc\n");
//Filipp swap 0 and 1 in std::get
    if(std::get<1>(ctx.Stack) < memory_need || std::get<1>(ctx.Stack) > 2*memory_need) {
        delete [] std::get<0>(ctx.Stack);
        std::get<1>(ctx.Stack) = memory_need;
        std::get<0>(ctx.Stack) = new char[memory_need];
    }

    printf("Exit heap alloc\n");
    memcpy(std::get<0>(ctx.Stack), ctx.Low, memory_need);
    printf("Memcpy_end\n");
}

void Engine::Restore(context &ctx) {
    char cur_addr;
    while(&cur_addr < ctx.Hight) {
        Restore(ctx);
    }

    assert(&cur_addr > ctx.Low);

    printf("Restore_before memcpy\n");
    std::size_t memory_restore = ctx.Hight - ctx.Low;
    memcpy(ctx.Low, std::get<0>(ctx.Stack), memory_restore);

    printf("Restore_after memcpy\n");

    cur_routine = &ctx;

    longjmp(ctx.Environment, 1);

}

void Engine::yield() {
    sched(nullptr);
}

void Engine::sched(void *routine_) {

    //TODO: nullptr
    if (setjmp(cur_routine->Environment) > 0) {
        return;
    }
    Store(*cur_routine);

    Restore(*(context*)routine_);
}

} // namespace Coroutine
} // namespace Afina

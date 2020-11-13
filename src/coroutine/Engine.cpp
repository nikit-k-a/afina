#include <afina/coroutine/Engine.h>

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#include <cassert>

namespace Afina {
namespace Coroutine {

void Engine::Store(context &ctx) {
    char addr = 0;
    ctx.Low = &addr; // higher on stack

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
    // printf("restore addr = %p\n", &cur_addr);
    // printf ("Low = %p\n", ctx.Low);
    while(&cur_addr > ctx.Low) {
        Restore(ctx);
    }

    assert(&cur_addr < ctx.Hight);

    printf("Restore_before memcpy\n");
    std::size_t memory_restore = ctx.Hight - ctx.Low;
    memcpy(ctx.Low, std::get<0>(ctx.Stack), memory_restore);

    printf("Restore_after memcpy\n");

    cur_routine = &ctx;

    longjmp(ctx.Environment, 1);
}

/*
void Engine::yield() {
    if (alive == nullptr) {
        return;
    }
    if (alive == cur_routine && alive->next != nullptr) {
        sched(alive->next);
    }
    sched(alive);
}

void Engine::sched(void *routine_) {
    context &ctx = *(static_cast<context *>(routine_));
    if(cur_routine == nullptr){
        return;
    }
    if (cur_routine != idle_ctx) { // Function was called from scheduller
        if (setjmp(cur_routine->Environment) > 0) {
            return;
        } // corutine continue
        Store(*cur_routine);       // Because storing of idle_ctx presented in start function
    }

    Restore(ctx);
}*/

void Engine::yield() {
    sched(nullptr);
}

void Engine::sched(void *routine_) {

    printf("sched enter\n");
    auto routine = (context*)routine_;

    if(alive == nullptr || routine_ == cur_routine) {
        printf("If 1\n");
        return;
    }


    if (routine == nullptr){
        if (alive == cur_routine && alive->next != nullptr) {
            printf("If 2\n");
            routine_ = alive->next;
        }
        routine = alive;
        printf("sched routine reset\n");
    }
    printf("sched before setjmp\n");
    //TODO: nullptr
    if(cur_routine != idle_ctx) {
        printf("If 4\n");
        if (setjmp(cur_routine->Environment) > 0) {
            printf("in setjmp\n");
            return;
        }
        printf("sched before store\n");
        Store(*cur_routine);
    }

    printf("sched before restore\n");
    Restore(*routine);
}

} // namespace Coroutine
} // namespace Afina

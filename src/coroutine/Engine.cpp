#include <afina/coroutine/Engine.h>

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#include <cassert>

namespace Afina {
namespace Coroutine {

void Engine::Store(context &ctx) {
    char addr = 0;
    if(&ctx != idle_ctx){
        ctx.Low = &addr; // higher on stack
    }

    assert (ctx.Hight - ctx.Low >= 0);

    std::size_t memory_need = ctx.Hight - ctx.Low;

    if(std::get<1>(ctx.Stack) < memory_need || std::get<1>(ctx.Stack) > 2*memory_need) {
        delete [] std::get<0>(ctx.Stack);
        std::get<1>(ctx.Stack) = memory_need;
        std::get<0>(ctx.Stack) = new char[memory_need];
    }

    if(&ctx != idle_ctx) {
        memcpy(std::get<0>(ctx.Stack), ctx.Low, memory_need);
    }
}

void Engine::Restore(context &ctx) {
    char cur_addr;
    while(&cur_addr > ctx.Low) {
        Restore(ctx);
    }

    assert(&cur_addr < ctx.Hight);

    std::size_t memory_restore = ctx.Hight - ctx.Low;

    if (&ctx != idle_ctx) {
        memcpy(ctx.Low, std::get<0>(ctx.Stack), memory_restore);
    }

    cur_routine = &ctx;
    longjmp(ctx.Environment, 1);
}

void Engine::yield() {
    sched(nullptr);
}

void Engine::sched(void *routine_) {
    auto routine = (context*)routine_;

    //nothing to do cases
    if(   alive == nullptr
       || routine_ == cur_routine
       || ( routine_ == blocked
           && routine_ != nullptr )) {
        return;
    }

    //yield case
    if (routine == nullptr){
        //assign next or alive to routine
        if (alive == cur_routine && alive->next != nullptr) {
            routine_ = alive->next;
        }
        //alive is not nullptr already
        //routine is now alive
        routine = alive;
    }

    if(cur_routine != idle_ctx) {
        if (setjmp(cur_routine->Environment) > 0) {
            return;
        }

        Store(*cur_routine);
    }

    Restore(*routine);
}

void Engine::remove_head(context*& head, context*& elmt) {
    //removing coroutine from list
    if(head == elmt) {
        head = head->next;
    }
    if (elmt->prev != nullptr) {
        elmt->prev->next = elmt->next;
    }
    if (elmt->next != nullptr) {
        elmt->next->prev = elmt->prev;
    }
}

void Engine::add_head(context*& head, context*& new_head) {
    if(head == nullptr) {
        head = new_head;
        head->prev = nullptr;
        head->next = nullptr;

    } else {
        new_head->prev = nullptr;

        if(head != nullptr) {
            head->prev = new_head;
        }
        new_head->next = head;
        head = new_head;
    }
}

void Engine::block(void *coro){
    auto blocking = (context*)coro;

    if (coro == nullptr){
        blocking = cur_routine;
    }
    //removing from alive list
    remove_head(alive, blocking);
    //adding to block list head
    add_head(blocked, blocking);

    if(blocking == cur_routine) {
        yield();
    }
}

void Engine::unblock(void* coro){
    auto unblocking = (context*)coro;
    if (unblocking == nullptr){
        return;
    }
    remove_head(blocked, unblocking);
    add_head(alive, unblocking);
}

} // namespace Coroutine
} // namespace Afina

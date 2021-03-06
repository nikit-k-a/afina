#include "gtest/gtest.h"

#include <iostream>
#include <sstream>

#include <afina/coroutine/Engine.h>

void _calculator_add(int &result, int left, int right) { result = left + right; }

TEST(CoroutineTest, SimpleStart) {
    Afina::Coroutine::Engine engine;

    int result;
    engine.start(_calculator_add, result, 1, 2);

    ASSERT_EQ(3, result);
}

void printa(Afina::Coroutine::Engine &pe, std::stringstream &out, void *&other, bool &block) {
    out << "A1 ";
    pe.sched(other);

    out << "A2 ";
    if(block) {
        pe.block(other);
    }

    pe.sched(other);

    if(block){
        pe.unblock(other);
    }

    out << "A3 ";
    pe.sched(other);
}

void printb(Afina::Coroutine::Engine &pe, std::stringstream &out, void *&other, bool &block) {
    out << "B1 ";
    pe.sched(other);

    out << "B2 ";
    pe.sched(other);

    out << "B3 ";
}

std::stringstream out;
void *pa = nullptr, *pb = nullptr;
void _printer(Afina::Coroutine::Engine &pe, std::string &result, bool &block) {
    // Create routines, note it doens't get control yet
    pa = pe.run(printa, pe, out, pb, block);
    pb = pe.run(printb, pe, out, pa, block);

    // Pass control to first routine, it will ping pong
    // between printa/printb greedely then we will get
    // control back
    pe.sched(pa);

    out << "END";

    // done
    result = out.str();
}

TEST(CoroutineTest, Printer) {
    Afina::Coroutine::Engine engine;
    bool block = false;
    std::string result = "";
    engine.start(_printer, engine, result, block);
    ASSERT_STREQ("A1 B1 A2 B2 A3 B3 END", result.c_str());
    std::stringstream().swap(out);
}


TEST(CoroutineTest, Blocker) {
    Afina::Coroutine::Engine engine;
    bool block = true;
    std::string result = "";
    engine.start(_printer, engine, result, block);
    ASSERT_STREQ("A1 B1 A2 A3 B2 B3 END", result.c_str());
}

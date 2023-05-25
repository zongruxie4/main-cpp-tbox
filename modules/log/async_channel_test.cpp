#include <gtest/gtest.h>
#include "async_channel.h"
#include <iostream>
#include <chrono>
#include <algorithm>

using namespace std;
using namespace tbox::log;

class TestAsyncChannel : public AsyncChannel {
  protected:
    virtual void appendLog(const char *str, size_t len) {
        cout << str << endl;
    }
};

class EmptyTestAsyncChannel : public AsyncChannel {
  protected:
    virtual void appendLog(const char *str, size_t len) { }
};


TEST(AsyncChannel, Format)
{
    TestAsyncChannel ch;

    ch.enable();

    LogInfo("%s, %d, %f", "hello", 123456, 12.345);
    LogInfo("%d, %f, %s", 123456, 12.345, "world");

    ch.cleanup();
}

TEST(AsyncChannel, LongString)
{
    TestAsyncChannel ch;

    ch.enable();
    std::string tmp(4096, 'x');
    LogInfo("%s", tmp.c_str());

    ch.cleanup();
}

#include <tbox/event/loop.h>
using namespace tbox::event;

TEST(AsyncChannel, Benchmark)
{
    TestAsyncChannel ch;
    ch.enable();
    std::string tmp(30, 'x');

    auto sp_loop = Loop::New();

    int counter = 0;
    function<void()> func = [&] {
        for (int i = 0; i < 100; ++i)
            LogInfo("%d %s", i, tmp.c_str());
        sp_loop->runInLoop(func);
        counter += 100;
    };
    sp_loop->runInLoop(func);

    sp_loop->exitLoop(chrono::seconds(10));
    sp_loop->runLoop();

    delete sp_loop;
    cout << "count in sec: " << counter/10 << endl;
    ch.cleanup();
}

TEST(AsyncChannel, Benchmark_Empty)
{
    EmptyTestAsyncChannel ch;
    ch.enable();
    std::string tmp(30, 'x');

    auto sp_loop = Loop::New();

    int counter = 0;
    function<void()> func = [&] {
        for (int i = 0; i < 100; ++i)
            LogInfo("%d %s", i, tmp.c_str());
        sp_loop->run(func);
        counter += 100;
    };
    sp_loop->run(func);

    sp_loop->exitLoop(chrono::seconds(10));
    sp_loop->runLoop();

    delete sp_loop;
    cout << "count in sec: " << counter/10 << endl;
    ch.cleanup();
}


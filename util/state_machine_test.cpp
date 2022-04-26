#include <gtest/gtest.h>

#include "state_machine.h"

using namespace std;
using namespace tbox::util;

enum State {
    STATE_A = 1,
    STATE_B,
    STATE_C,
    STATE_D,
};

enum Event {
    EVENT_1 = 1,
    EVENT_2,
    EVENT_3,
    EVENT_4,
};

//! 测试创建状态
TEST(StateMachine, NewState)
{
    StateMachine sm;
    EXPECT_TRUE (sm.newState(STATE_A, nullptr, nullptr));
    EXPECT_TRUE (sm.newState(STATE_B, nullptr, nullptr));
    EXPECT_FALSE(sm.newState(STATE_A, nullptr, nullptr));
}

//! 测试创建事件
TEST(StateMachine, AddRoute)
{
    StateMachine sm;
    sm.newState(STATE_A, nullptr, nullptr);
    sm.newState(STATE_B, nullptr, nullptr);

    EXPECT_TRUE (sm.addRoute(STATE_A, EVENT_1, STATE_B, nullptr, nullptr));
    EXPECT_FALSE(sm.addRoute(STATE_A, EVENT_1, STATE_C, nullptr, nullptr));
    EXPECT_FALSE(sm.addRoute(STATE_C, EVENT_1, STATE_B, nullptr, nullptr));
    EXPECT_FALSE(sm.addRoute(STATE_C, EVENT_1, STATE_D, nullptr, nullptr));
}

//! 测试创建两个状态，一个事件由A到B转换，没有守卫与执行函数
TEST(StateMachine, StateWithoutGuardAndAction)
{
    StateMachine sm;
    sm.newState(STATE_A, nullptr, nullptr);
    sm.newState(STATE_B, nullptr, nullptr);
    sm.addRoute(STATE_A, EVENT_1, STATE_B, nullptr, nullptr);
    sm.setInitState(STATE_A);
    sm.start();

    EXPECT_EQ(sm.currentState(), STATE_A);
    EXPECT_TRUE(sm.run(EVENT_1));
    EXPECT_EQ(sm.currentState(), STATE_B);
    EXPECT_FALSE(sm.run(EVENT_2));
}

//! 测试创建两个状态，一个事件由A到B转换，有守卫
TEST(StateMachine, RouteWithGuard)
{
    StateMachine sm;
    sm.newState(STATE_A, nullptr, nullptr);
    sm.newState(STATE_B, nullptr, nullptr);
    sm.newState(STATE_C, nullptr, nullptr);

    bool condition = false;
    sm.addRoute(STATE_A, EVENT_1, STATE_B, [&]{ return !condition; }, nullptr);
    sm.addRoute(STATE_A, EVENT_1, STATE_C, [&]{ return condition; },  nullptr);
    sm.addRoute(STATE_B, EVENT_2, STATE_A, nullptr, nullptr);
    sm.addRoute(STATE_C, EVENT_2, STATE_A, nullptr, nullptr);
    sm.setInitState(STATE_A);
    sm.start();

    EXPECT_EQ(sm.currentState(), STATE_A);

    condition = false;
    EXPECT_TRUE(sm.run(EVENT_1));
    EXPECT_EQ(sm.currentState(), STATE_B);

    EXPECT_TRUE(sm.run(EVENT_2));
    EXPECT_EQ(sm.currentState(), STATE_A);

    condition = true;
    EXPECT_TRUE(sm.run(EVENT_1));
    EXPECT_EQ(sm.currentState(), STATE_C);

    EXPECT_TRUE(sm.run(EVENT_2));
    EXPECT_EQ(sm.currentState(), STATE_A);
}

//! 测试创建两个状态，一个事件由A到B转换，有进入与退出动作
TEST(StateMachine, StateWithEnterAndExitAction)
{
    StateMachine sm;

    int a_enter_counter = 0;
    int a_exit_counter = 0;
    int b_enter_counter = 0;
    int b_exit_counter = 0;

    sm.newState(STATE_A, [&] { ++a_enter_counter; }, [&] { ++a_exit_counter; });
    sm.newState(STATE_B, [&] { ++b_enter_counter; }, [&] { ++b_exit_counter; });

    sm.addRoute(STATE_A, EVENT_1, STATE_B, nullptr, nullptr);
    sm.addRoute(STATE_B, EVENT_2, STATE_A, nullptr, nullptr);

    sm.setInitState(STATE_A);
    EXPECT_EQ(a_enter_counter, 0);
    sm.start();
    EXPECT_EQ(a_enter_counter, 1);

    sm.run(EVENT_1);
    EXPECT_EQ(a_exit_counter, 1);
    EXPECT_EQ(b_enter_counter, 1);

    sm.run(EVENT_2);
    EXPECT_EQ(b_exit_counter, 1);
    EXPECT_EQ(a_enter_counter, 2);
}

//! 测试创建两个状态，一个事件由A到B转换，事件有转换动作
TEST(StateMachine, EventWithAction)
{
    StateMachine sm;

    sm.newState(STATE_A, nullptr, nullptr);
    sm.newState(STATE_B, nullptr, nullptr);

    int count_1 = 0;
    int count_2 = 0;

    sm.addRoute(STATE_A, EVENT_1, STATE_B, nullptr, [&]{ ++count_1; });
    sm.addRoute(STATE_B, EVENT_2, STATE_A, nullptr, [&]{ ++count_2; });
    sm.setInitState(STATE_A);
    sm.start();

    sm.run(EVENT_1);
    EXPECT_EQ(count_1, 1);

    sm.run(EVENT_2);
    EXPECT_EQ(count_2, 1);
}

TEST(StateMachine, AnyEvent)
{
    StateMachine sm;

    sm.newState(STATE_A, nullptr, nullptr);
    sm.newState(STATE_B, nullptr, nullptr);

    sm.addRoute(STATE_A, 0, STATE_B, nullptr, nullptr);
    sm.addRoute(STATE_B, 0, STATE_A, nullptr, nullptr);
    sm.setInitState(STATE_A);
    sm.start();

    sm.run(EVENT_1);
    EXPECT_EQ(sm.currentState(), STATE_B);
    sm.run(EVENT_2);
    EXPECT_EQ(sm.currentState(), STATE_A);
    sm.run(EVENT_3);
    EXPECT_EQ(sm.currentState(), STATE_B);
}

TEST(StateMachine, Restart)
{
    StateMachine sm;

    int a_enter_counter = 0;
    int a_exit_counter = 0;
    int b_enter_counter = 0;
    int b_exit_counter = 0;

    sm.newState(STATE_A, [&] { ++a_enter_counter; }, [&] { ++a_exit_counter; });
    sm.newState(STATE_B, [&] { ++b_enter_counter; }, [&] { ++b_exit_counter; });

    sm.addRoute(STATE_A, EVENT_1, STATE_B, nullptr, nullptr);
    sm.setInitState(STATE_A);
    sm.start();
    EXPECT_EQ(a_enter_counter, 1);
    EXPECT_TRUE(sm.run(EVENT_1));
    EXPECT_EQ(a_exit_counter, 1);
    EXPECT_EQ(b_enter_counter, 1);
    sm.stop();
    EXPECT_EQ(b_exit_counter, 1);
    sm.start();
    EXPECT_EQ(a_enter_counter, 2);
}

//! 主要测试用 enum class 定义的状态与事件是否能编译过
TEST(StateMachine, EnumClass)
{
    StateMachine sm;
    enum class State { k1 = 1, k2 };
    enum class Event { k1 = 1, k2 };

    int enter_counter = 0;
    int exit_counter = 0;

    sm.newState(State::k1, [&] { ++enter_counter; }, [&] { ++exit_counter; });
    sm.newState(State::k2, nullptr, nullptr);

    sm.addRoute(State::k1, Event::k1, State::k2, nullptr, nullptr);
    sm.addRoute(State::k2, Event::k2, State::k1, nullptr, nullptr);
    sm.setInitState(State::k1);
    sm.start();
    sm.run(Event::k1);
    sm.run(Event::k2);
    sm.stop();

    EXPECT_EQ(enter_counter, 2);
    EXPECT_EQ(exit_counter, 2);
}

TEST(StateMachine, SubStateMachine)
{
    enum class State { kTerm, kInit, k1, k2 };
    enum class Event { kTerm, k1, k2 };

    StateMachine sm, sub_sm;

    sm.newState(State::kInit, nullptr, nullptr);
    sm.newState(State::k1,    nullptr, nullptr);
    sm.newState(State::k2,    nullptr, nullptr);

    sm.addRoute(State::kInit, Event::k1, State::k1,    nullptr, nullptr);
    sm.addRoute(State::kInit, Event::k2, State::k2,    nullptr, nullptr);
    sm.addRoute(State::k1,    Event::k1, State::kTerm, nullptr, nullptr);
    sm.addRoute(State::k1,    Event::k2, State::k2,    nullptr, nullptr);
    sm.addRoute(State::k2,    Event::k1, State::k1,    nullptr, nullptr);
    sm.addRoute(State::k2,    Event::k2, State::k1,    nullptr, nullptr);

    sub_sm.newState(State::kInit, nullptr, nullptr);
    sub_sm.newState(State::k1,    nullptr, nullptr);
    sub_sm.newState(State::k2,    nullptr, nullptr);

    sub_sm.addRoute(State::kInit, Event::k1, State::k1,    nullptr, nullptr);
    sub_sm.addRoute(State::kInit, Event::k2, State::k2,    nullptr, nullptr);
    sub_sm.addRoute(State::k1,    Event::k2, State::k2,    nullptr, nullptr);
    sub_sm.addRoute(State::k1,    Event::k1, State::kTerm, nullptr, nullptr);
    sub_sm.addRoute(State::k2,    Event::k1, State::k1,    nullptr, nullptr);
    sub_sm.addRoute(State::k2,    Event::k2, State::kTerm, nullptr, nullptr);

    sm.setSubStateMachine(State::k1, &sub_sm);

    {   //! 直通
        sm.start();

        EXPECT_EQ(sm.currentState<State>(), State::kInit);
        EXPECT_EQ(sub_sm.currentState<State>(), State::kTerm);

        EXPECT_TRUE(sm.run(Event::k1));
        EXPECT_EQ(sm.currentState<State>(), State::k1);
        EXPECT_EQ(sub_sm.currentState<State>(), State::k1);

        EXPECT_TRUE(sm.run(Event::k1));
        EXPECT_EQ(sm.currentState<State>(), State::kTerm);
        EXPECT_EQ(sub_sm.currentState<State>(), State::kTerm);

        EXPECT_TRUE(sm.isTerminated());

        sm.stop();
    }

    {   //! 在子状态机里转了一下
        sm.start();

        EXPECT_EQ(sm.currentState<State>(), State::kInit);
        EXPECT_EQ(sub_sm.currentState<State>(), State::kTerm);

        EXPECT_TRUE(sm.run(Event::k1));
        EXPECT_EQ(sm.currentState<State>(), State::k1);
        EXPECT_EQ(sub_sm.currentState<State>(), State::k1);

        EXPECT_TRUE(sm.run(Event::k2));
        EXPECT_EQ(sm.currentState<State>(), State::k1);
        EXPECT_EQ(sub_sm.currentState<State>(), State::k2);

        EXPECT_TRUE(sm.run(Event::k1));
        EXPECT_EQ(sm.currentState<State>(), State::k1);
        EXPECT_EQ(sub_sm.currentState<State>(), State::k1);

        EXPECT_TRUE(sm.run(Event::k1));
        EXPECT_EQ(sm.currentState<State>(), State::kTerm);
        EXPECT_EQ(sub_sm.currentState<State>(), State::kTerm);
        EXPECT_TRUE(sm.isTerminated());

        sm.stop();
    }

    {   //! 从S2进的S1的子状态机，再出来回到S2，再进去
        sm.start();

        EXPECT_EQ(sm.currentState<State>(), State::kInit);
        EXPECT_EQ(sub_sm.currentState<State>(), State::kTerm);

        EXPECT_TRUE(sm.run(Event::k2));
        EXPECT_EQ(sm.currentState<State>(), State::k2);
        EXPECT_EQ(sub_sm.currentState<State>(), State::kTerm);

        EXPECT_TRUE(sm.run(Event::k2));
        EXPECT_EQ(sm.currentState<State>(), State::k1);
        EXPECT_EQ(sub_sm.currentState<State>(), State::k2);

        EXPECT_TRUE(sm.run(Event::k2));
        EXPECT_EQ(sm.currentState<State>(), State::k2);
        EXPECT_EQ(sub_sm.currentState<State>(), State::kTerm);

        EXPECT_TRUE(sm.run(Event::k2));
        EXPECT_EQ(sm.currentState<State>(), State::k1);
        EXPECT_EQ(sub_sm.currentState<State>(), State::k2);

        EXPECT_TRUE(sm.run(Event::k1));
        EXPECT_EQ(sm.currentState<State>(), State::k1);
        EXPECT_EQ(sub_sm.currentState<State>(), State::k1);

        EXPECT_TRUE(sm.run(Event::k1));
        EXPECT_EQ(sm.currentState<State>(), State::kTerm);
        EXPECT_EQ(sub_sm.currentState<State>(), State::kTerm);

        sm.stop();
    }
}

TEST(StateMachine, StateChangedCallback)
{
    enum class State { kTerm, kInit, k1 };
    enum class Event { kNone, k1 };

    StateMachine sm;

    sm.newState(State::kInit, nullptr, nullptr);
    sm.newState(State::k1,    nullptr, nullptr);

    sm.addRoute(State::kInit, Event::k1, State::k1,    nullptr, nullptr);
    sm.addRoute(State::k1,    Event::k1, State::kTerm, nullptr, nullptr);

    State from, to;
    Event event;

    sm.setStateChangedCallback(
        [&] (StateMachine::StateID f, StateMachine::StateID t, StateMachine::EventID e) {
            from = static_cast<State>(f);
            to = static_cast<State>(t);
            event = static_cast<Event>(e);
        }
    );
    sm.start();
    sm.run(Event::k1);
    EXPECT_EQ(from, State::kInit);
    EXPECT_EQ(to, State::k1);
    EXPECT_EQ(event, Event::k1);

    sm.run(Event::k1);
    EXPECT_EQ(from, State::k1);
    EXPECT_EQ(to, State::kTerm);
    EXPECT_EQ(event, Event::k1);
}

TEST(StateMachine, SetInitState)
{
    enum class State { kTerm, k1, kInit };
    enum class Event { kNone, k1 };

    StateMachine sm;

    sm.setInitState(State::kInit);
    sm.newState(State::kInit, nullptr, nullptr);
    sm.newState(State::k1,    nullptr, nullptr);

    EXPECT_TRUE(sm.start());
    EXPECT_EQ(sm.currentState<State>(), State::kInit);
}

TEST(StateMachine, SetInitState_Fail)
{
    enum class State { kTerm, k1, kInit };
    enum class Event { kNone, k1 };

    StateMachine sm;

    sm.setInitState(State::kInit);
    sm.newState(State::k1,    nullptr, nullptr);

    EXPECT_FALSE(sm.start());
}

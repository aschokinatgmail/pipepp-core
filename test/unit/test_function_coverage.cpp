#include <gtest/gtest.h>
#include <pipepp/core/any_source.hpp>
#include <pipepp/core/source_registry.hpp>
#include <pipepp/core/pipeline.hpp>
#include <pipepp/core/pipeline_ops.hpp>
#include <pipepp/core/connect_to.hpp>
#include <pipepp/core/stages.hpp>
#include <pipepp/core/adapt.hpp>
#include <pipepp/core/small_function.hpp>
#include <pipepp/core/message.hpp>
#include "../helpers/test_sources.hpp"

#include <thread>
#include <array>
#include <vector>

using namespace pipepp::core;

TEST(FunctionCovTest, AnySourceEmbeddedConfigCountingSource) {
    any_source<embedded_config> src(CountingSource{});
    EXPECT_TRUE(src.connect().has_value());
    EXPECT_TRUE(src.is_connected());
    EXPECT_TRUE(src.subscribe("t", 0).has_value());
    std::byte d[] = {std::byte{0x01}};
    EXPECT_TRUE(src.publish("t", d, 0).has_value());
    src.set_message_callback(message_callback<embedded_config>(NoopCallback{}));
    src.poll();
    EXPECT_TRUE(src.disconnect().has_value());
}

TEST(FunctionCovTest, AnySourceEmbeddedConfigMove) {
    any_source<embedded_config> src1(CountingSource{});
    src1.connect();
    any_source<embedded_config> src2 = std::move(src1);
    EXPECT_TRUE(src2.is_connected());
    EXPECT_TRUE(src2.disconnect().has_value());
}

TEST(FunctionCovTest, AnySourceEmbeddedConfigMoveAssign) {
    any_source<embedded_config> src1(CountingSource{});
    src1.connect();
    any_source<embedded_config> src2;
    src2 = std::move(src1);
    EXPECT_TRUE(src2.is_connected());
    src2.disconnect();
}

TEST(FunctionCovTest, AnySourceDefaultConfigMoveImplCounting) {
    any_source<default_config> src1(CountingSource{});
    src1.connect();
    any_source<default_config> src2 = std::move(src1);
    EXPECT_TRUE(src2.is_connected());
    src2.disconnect();
}

TEST(FunctionCovTest, AnySourceDefaultConfigMoveImplSelfStopping) {
    any_source<default_config> src1(SelfStoppingSource{});
    src1.connect();
    any_source<default_config> src2 = std::move(src1);
    EXPECT_TRUE(src2.is_connected());
    src2.disconnect();
}

TEST(FunctionCovTest, IdleSourceEmitAndProcess) {
    basic_pipeline<MockSource> pipe(MockSource{});
    int sink_count = 0;
    pipe.add_stage(TrueStage{});
    pipe.set_sink(CountingSink{&sink_count});
    pipe.check();

    std::byte data[] = {std::byte{0x01}};
    message_view mv("test/emit", data, 0);
    pipe.emit(mv);
    EXPECT_EQ(sink_count, 1);
}

TEST(FunctionCovTest, IdleSourceEventLoop) {
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.add_stage(TrueStage{});
    pipe.set_sink(NoopSink{});

    std::thread runner([&]() { (void)pipe.run(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    pipe.stop();
    runner.join();
}

TEST(FunctionCovTest, FailingConnectSourceEmit) {
    FaultSource src;
    src.fail_connect = true;
    basic_pipeline<FaultSource> pipe(std::move(src));
    pipe.add_stage(TrueStage{});
    pipe.set_sink(NoopSink{});
    auto r = pipe.run();
    EXPECT_FALSE(r.has_value());
}

TEST(FunctionCovTest, FailingSubscribeSourceEmit) {
    FaultSource src;
    src.fail_subscribe = true;
    src.max_polls = 1;
    basic_pipeline<FaultSource> pipe(std::move(src));
    pipe.add_stage(TrueStage{});
    int sink_count = 0;
    pipe.set_sink(CountingSink{&sink_count});
    pipe.subscribe("test/topic", 0);

    std::thread runner([&]() { (void)pipe.run(); });
    runner.join();
}

TEST(FunctionCovTest, MockSourceEmitAndRun) {
    basic_pipeline<MockSource> pipe(MockSource{});
    int sink_count = 0;
    pipe.add_stage(TrueStage{});
    pipe.set_sink(CountingSink{&sink_count});
    pipe.check();

    std::byte data[] = {std::byte{0x01}};
    message_view mv("mock/emit", data, 0);
    pipe.emit(mv);
    EXPECT_GT(sink_count, 0);
}

TEST(FunctionCovTest, MockSourceEventLoop) {
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.add_stage(TrueStage{});
    pipe.set_sink(NoopSink{});

    std::thread runner([&]() { (void)pipe.run(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    pipe.stop();
    runner.join();
}

TEST(FunctionCovTest, EmbeddedConfigPipelineFullRun) {
    basic_pipeline<SelfStoppingSource, embedded_config> pipe(SelfStoppingSource{});
    pipe.add_stage(TrueStage{});
    int sink_count = 0;
    pipe.set_sink(CountingSink{&sink_count});

    auto r = pipe.run();
    EXPECT_TRUE(r.has_value());
    EXPECT_GT(sink_count, 0);
}

TEST(FunctionCovTest, SmallFunctionMoveAndInvoke) {
    small_function<int(int), 64> fn1 = [](int x) { return x + 1; };
    small_function<int(int), 64> fn2 = std::move(fn1);
    EXPECT_EQ(fn2(5), 6);

    small_function<int(int), 64> fn3;
    fn3 = std::move(fn2);
    EXPECT_EQ(fn3(10), 11);
}

TEST(FunctionCovTest, SmallFunctionVoidInvoke) {
    bool called = false;
    small_function<void(), 64> fn1 = [&]() { called = true; };
    small_function<void(), 64> fn2 = std::move(fn1);
    fn2();
    EXPECT_TRUE(called);
}

TEST(FunctionCovTest, SmallFunctionSinkInvoke) {
    bool called = false;
    small_function<void(const message_view&), 128> fn1 = [&](const message_view&) { called = true; };
    small_function<void(const message_view&), 128> fn2 = std::move(fn1);
    std::byte data[] = {std::byte{0x01}};
    message_view mv("test", data, 0);
    fn2(mv);
    EXPECT_TRUE(called);
}

TEST(FunctionCovTest, SmallFunctionProcessInvoke) {
    small_function<bool(basic_message<default_config>&), 128> fn1 = [](basic_message<default_config>&) { return true; };
    small_function<bool(basic_message<default_config>&), 128> fn2 = std::move(fn1);
    basic_message<default_config> msg;
    EXPECT_TRUE(fn2(msg));
}

TEST(FunctionCovTest, SourceRegistryEmbeddedConfig) {
    source_registry<embedded_config> reg;
    reg.register_scheme<CountingSource>("cnt");
    auto src = reg.create("cnt://host/path");
    ASSERT_TRUE(static_cast<bool>(src));
    EXPECT_TRUE(src.connect().has_value());
    src.disconnect();
}

TEST(FunctionCovTest, PipelineOpsWithProcessAndSink) {
    basic_pipeline<CountingSource> p1(CountingSource{});
    int sink_count = 0;
    auto p2 = std::move(p1)
        | process(TrueStage{})
        | sink(CountingSink{&sink_count});
    p2.check();

    std::byte data[] = {std::byte{0x01}};
    message_view mv("ops/test", data, 0);
    p2.emit(mv);
    EXPECT_GT(sink_count, 0);
}

TEST(FunctionCovTest, PipelineOpsWithConfigureAndRun) {
    int sink_count = 0;
    basic_pipeline<CountingSource> p1(CountingSource{});
    auto p2 = std::move(p1)
        | configure(NoopConfigure{})
        | process(TrueStage{})
        | sink(CountingSink{&sink_count});
    p2.check();

    std::byte data[] = {std::byte{0x01}};
    message_view mv("cfg/test", data, 0);
    p2.emit(mv);
    EXPECT_GT(sink_count, 0);
}

TEST(FunctionCovTest, PipelineOpsFilterAndSink) {
    basic_pipeline<MockSource> p1(MockSource{});
    int sink_count = 0;
    auto p2 = std::move(p1)
        | filter(TrueFilter{})
        | sink(CountingSink{&sink_count});
    p2.check();

    std::byte data[] = {std::byte{0x01}};
    message_view mv("flt/test", data, 0);
    p2.emit(mv);
    EXPECT_GT(sink_count, 0);
}

TEST(FunctionCovTest, PipelineOpsSubscribeAndProcess) {
    basic_pipeline<CountingSource> p1(CountingSource{});
    int sink_count = 0;
    auto p2 = std::move(p1)
        | subscribe("sub/topic", 1)
        | process(TrueStage{})
        | sink(CountingSink{&sink_count});
    p2.check();

    std::byte data[] = {std::byte{0x01}};
    message_view mv("sub/test", data, 0);
    p2.emit(mv);
    EXPECT_GT(sink_count, 0);
}

TEST(FunctionCovTest, AdaptWithCallbackInvoked) {
    std::byte data[] = {std::byte{0x01}};
    std::array<message_view, 1> arr = {message_view("adapt/test", data, 0)};
    auto src = adapt(arr);
    src.connect();
    int cb_count = 0;
    src.set_message_callback(message_callback<default_config>(CountingCallback{&cb_count}));
    src.poll();
    EXPECT_GT(cb_count, 0);
}

TEST(FunctionCovTest, AdaptSubscribeCalled) {
    std::array<message_view, 0> arr;
    auto src = adapt(arr);
    EXPECT_TRUE(src.subscribe("topic", 0).has_value());
}

TEST(FunctionCovTest, PipelineConfigureMockSource) {
    basic_pipeline<SelfStoppingSource> pipe(SelfStoppingSource{});
    bool config_called = false;
    pipe.configure([&](SelfStoppingSource&) -> result { config_called = true; return {}; });
    pipe.add_stage(TrueStage{});
    pipe.set_sink(NoopSink{});
    auto r = pipe.run();
    EXPECT_TRUE(r.has_value());
    EXPECT_TRUE(config_called);
}

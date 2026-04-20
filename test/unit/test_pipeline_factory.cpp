#include <gtest/gtest.h>
#include <pipepp/core/pipeline_factory.hpp>
#include <pipepp/core/pipeline_ops.hpp>
#include "../helpers/test_sources.hpp"

using namespace pipepp::core;

TEST(PipelineFactoryTest, RequiredSizeIsCorrect) {
    constexpr auto sz = pipeline_allocator<MockSource>::required_size;
    constexpr auto align = pipeline_allocator<MockSource>::required_align;
    EXPECT_GE(sz, sizeof(basic_pipeline<MockSource>));
    EXPECT_GE(align, alignof(basic_pipeline<MockSource>));
    EXPECT_GT(sz, 0u);
}

TEST(PipelineFactoryTest, CreateInStackBuffer) {
    alignas(pipeline_allocator<MockSource>::required_align)
        std::byte buffer[pipeline_allocator<MockSource>::required_size];

    auto* pipe = pipeline_allocator<MockSource>::create(buffer, sizeof(buffer), MockSource{});
    ASSERT_NE(pipe, nullptr);
    EXPECT_FALSE(pipe->is_running());

    pipe->add_stage([](basic_message<default_config>&) { return true; });
    auto check = pipe->check();
    EXPECT_TRUE(check.has_value());

    pipeline_allocator<MockSource>::destroy(pipe);
}

TEST(PipelineFactoryTest, CreateWithUri) {
    alignas(pipeline_allocator<MockSource>::required_align)
        std::byte buffer[pipeline_allocator<MockSource>::required_size];

    auto* pipe = pipeline_allocator<MockSource>::create_uri(
        buffer, sizeof(buffer), MockSource{}, "mqtt://broker:1883/topic");
    ASSERT_NE(pipe, nullptr);

    pipeline_allocator<MockSource>::destroy(pipe);
}

TEST(PipelineFactoryTest, NullBufferReturnsNull) {
    auto* pipe = pipeline_allocator<MockSource>::create(nullptr, 1024, MockSource{});
    EXPECT_EQ(pipe, nullptr);
}

TEST(PipelineFactoryTest, SmallBufferReturnsNull) {
    alignas(std::max_align_t) std::byte buffer[16];
    auto* pipe = pipeline_allocator<MockSource>::create(buffer, sizeof(buffer), MockSource{});
    EXPECT_EQ(pipe, nullptr);
}

TEST(PipelineFactoryTest, FreeFunctionSizes) {
    EXPECT_EQ(pipeline_size<MockSource>(), sizeof(basic_pipeline<MockSource>));
    EXPECT_EQ(pipeline_align<MockSource>(), alignof(basic_pipeline<MockSource>));
}

TEST(PipelineFactoryTest, EmitViaPlacementNewPipeline) {
    alignas(pipeline_allocator<MockSource>::required_align)
        std::byte buffer[pipeline_allocator<MockSource>::required_size];

    bool sink_called = false;
    auto* pipe = pipeline_allocator<MockSource>::create(buffer, sizeof(buffer), MockSource{});
    ASSERT_NE(pipe, nullptr);
    pipe->add_stage([](basic_message<default_config>&) { return true; });
    pipe->set_sink([&](const message_view&) { sink_called = true; });

    std::byte data[] = {std::byte{0x01}};
    message_view mv("factory/test", data, 0);
    pipe->emit(mv);
    EXPECT_TRUE(sink_called);

    pipeline_allocator<MockSource>::destroy(pipe);
}

TEST(PipelineFactoryTest, EmbeddedConfigSize) {
    auto sz = pipeline_size<MockSource, embedded_config>();
    auto default_sz = pipeline_size<MockSource, default_config>();
    EXPECT_LT(sz, default_sz);
}

TEST(PipelineFactoryTest, MisalignedBufferReturnsNull) {
    std::byte buffer[pipeline_allocator<MockSource>::required_size + alignof(std::max_align_t)];
    auto* misaligned = buffer + 1;
    auto* pipe = pipeline_allocator<MockSource>::create(misaligned, pipeline_allocator<MockSource>::required_size, MockSource{});
    EXPECT_EQ(pipe, nullptr);
}

TEST(PipelineFactoryTest, CreateUriNullBufferReturnsNull) {
    auto* pipe = pipeline_allocator<MockSource>::create_uri(nullptr, 1024, MockSource{}, "mqtt://x/y");
    EXPECT_EQ(pipe, nullptr);
}

TEST(PipelineFactoryTest, CreateUriMisalignedBufferReturnsNull) {
    std::byte buffer[pipeline_allocator<MockSource>::required_size + alignof(std::max_align_t)];
    auto* misaligned = buffer + 1;
    auto* pipe = pipeline_allocator<MockSource>::create_uri(misaligned, pipeline_allocator<MockSource>::required_size, MockSource{}, "mqtt://x/y");
    EXPECT_EQ(pipe, nullptr);
}

TEST(PipelineFactoryTest, DestroyNullIsSafe) {
    pipeline_allocator<MockSource>::destroy(nullptr);
}

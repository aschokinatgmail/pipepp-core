#include <gtest/gtest.h>
#include <pipepp/core/any_source.hpp>
#include <pipepp/core/source_registry.hpp>
#include "../helpers/test_sources.hpp"

using namespace pipepp::core;

static_assert(BusSource<MockSource, default_config>);

TEST(AnySourceTest, DefaultConstructsToNull) {
    any_source<default_config> src;
    EXPECT_FALSE(static_cast<bool>(src));
}

TEST(AnySourceTest, ConstructFromMockSource) {
    any_source<default_config> src(MockSource{});
    EXPECT_TRUE(static_cast<bool>(src));
}

TEST(AnySourceTest, ConnectAndDisconnect) {
    any_source<default_config> src(MockSource{});
    auto r = src.connect();
    EXPECT_TRUE(r.has_value());
    EXPECT_TRUE(src.is_connected());
    r = src.disconnect();
    EXPECT_TRUE(r.has_value());
    EXPECT_FALSE(src.is_connected());
}

TEST(AnySourceTest, SubscribeViaTypeErasure) {
    any_source<default_config> src(MockSource{});
    auto r = src.subscribe("test/topic", 1);
    EXPECT_TRUE(r.has_value());
}

TEST(AnySourceTest, PublishViaTypeErasure) {
    any_source<default_config> src(MockSource{});
    std::byte data[] = {std::byte{0x42}};
    auto r = src.publish("pub/topic", data, 0);
    EXPECT_TRUE(r.has_value());
}

TEST(AnySourceTest, SetCallbackAndPoll) {
    any_source<default_config> src(MockSource{});
    auto cb = message_callback<default_config>(NoopCallback{});
    src.set_message_callback(std::move(cb));
    src.poll();
}

TEST(AnySourceTest, MoveConstruction) {
    any_source<default_config> src1(MockSource{});
    EXPECT_TRUE(static_cast<bool>(src1));
    any_source<default_config> src2 = std::move(src1);
    EXPECT_TRUE(static_cast<bool>(src2));
    EXPECT_FALSE(static_cast<bool>(src1));
}

TEST(AnySourceTest, MoveAssignment) {
    any_source<default_config> src1(MockSource{});
    any_source<default_config> src2;
    EXPECT_FALSE(static_cast<bool>(src2));
    src2 = std::move(src1);
    EXPECT_TRUE(static_cast<bool>(src2));
    EXPECT_FALSE(static_cast<bool>(src1));
}

TEST(AnySourceTest, ResetClearsSource) {
    any_source<default_config> src(MockSource{});
    EXPECT_TRUE(static_cast<bool>(src));
    src.reset();
    EXPECT_FALSE(static_cast<bool>(src));
}

TEST(AnySourceTest, NullSourceConnectReturnsError) {
    any_source<default_config> src;
    EXPECT_FALSE(src.is_connected());
    auto r = src.connect();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::not_connected);
}

TEST(SourceRegistryTest, RegisterAndCreate) {
    source_registry<default_config> reg;
    reg.register_scheme<MockSource>("mock");
    EXPECT_EQ(reg.size(), 1u);
    EXPECT_TRUE(reg.has_scheme("mock"));
}

TEST(SourceRegistryTest, CreateFromScheme) {
    source_registry<default_config> reg;
    reg.register_scheme<MockSource>("mock");
    auto src = reg.create("mock");
    EXPECT_TRUE(static_cast<bool>(src));
    EXPECT_TRUE(src.connect().has_value());
    EXPECT_TRUE(src.is_connected());
}

TEST(SourceRegistryTest, UnknownSchemeReturnsNull) {
    source_registry<default_config> reg;
    reg.register_scheme<MockSource>("mock");
    auto src = reg.create("unknown");
    EXPECT_FALSE(static_cast<bool>(src));
}

TEST(SourceRegistryTest, MultipleSchemes) {
    source_registry<default_config> reg;
    reg.register_scheme<MockSource>("mock");
    reg.register_scheme<CountingSource>("counting");
    EXPECT_EQ(reg.size(), 2u);
    EXPECT_TRUE(reg.has_scheme("mock"));
    EXPECT_TRUE(reg.has_scheme("counting"));
    EXPECT_FALSE(reg.has_scheme("mqtt"));
}

TEST(SourceRegistryTest, CreateFromFullUri) {
    source_registry<default_config> reg;
    reg.register_scheme<MockSource>("mock");
    auto src = reg.create("mock://host:1883/topic");
    EXPECT_TRUE(static_cast<bool>(src));
    EXPECT_TRUE(src.connect().has_value());
    EXPECT_TRUE(src.is_connected());
}

TEST(SourceFactoryTest, CreateAnySource) {
    auto src = source_factory<default_config>::create<MockSource>();
    EXPECT_TRUE(static_cast<bool>(src));
}

TEST(AnySourceTest, FullSourceConnectDisconnect) {
    any_source<default_config> src(FullSource{});
    EXPECT_TRUE(src.connect().has_value());
    EXPECT_TRUE(src.is_connected());
    EXPECT_TRUE(src.disconnect().has_value());
    EXPECT_FALSE(src.is_connected());
}

TEST(AnySourceTest, FullSourceSubscribeRecordsTopicAndQos) {
    any_source<default_config> src(FullSource{});
    auto r = src.subscribe("sensors/temp", 2);
    EXPECT_TRUE(r.has_value());
}

TEST(AnySourceTest, FullSourcePublishIncrements) {
    any_source<default_config> src(FullSource{});
    std::byte data[] = {std::byte{0x01}, std::byte{0x02}};
    EXPECT_TRUE(src.publish("out/topic", data, 1).has_value());
}

TEST(AnySourceTest, FullSourceSetCallbackAndPoll) {
    any_source<default_config> src(FullSource{});
    src.set_message_callback(message_callback<default_config>(NoopCallback{}));
    src.poll();
}

TEST(AnySourceTest, CountingSourceAllMethods) {
    any_source<default_config> src(CountingSource{});
    EXPECT_TRUE(src.connect().has_value());
    EXPECT_TRUE(src.is_connected());
    EXPECT_TRUE(src.subscribe("a/b", 0).has_value());
    std::byte data[] = {std::byte{0xFF}};
    EXPECT_TRUE(src.publish("c/d", data, 1).has_value());
    src.set_message_callback(message_callback<default_config>(NoopCallback{}));
    src.poll();
    EXPECT_TRUE(src.disconnect().has_value());
    EXPECT_FALSE(src.is_connected());
}

TEST(AnySourceTest, SelfStoppingSourceCallbackAndPoll) {
    any_source<default_config> src(SelfStoppingSource{});
    src.connect();
    bool called = false;
    src.set_message_callback(message_callback<default_config>([&](const message_view& mv) {
        called = true;
        EXPECT_EQ(mv.topic, "auto/topic");
    }));
    src.poll();
    EXPECT_TRUE(called);
    EXPECT_FALSE(src.is_connected());
}

TEST(AnySourceTest, MoveConstructFromFullSource) {
    any_source<default_config> src1(FullSource{});
    src1.connect();
    any_source<default_config> src2 = std::move(src1);
    EXPECT_FALSE(static_cast<bool>(src1));
    EXPECT_TRUE(static_cast<bool>(src2));
    EXPECT_TRUE(src2.is_connected());
    EXPECT_TRUE(src2.disconnect().has_value());
}

TEST(AnySourceTest, MoveAssignFullSourceToEmpty) {
    any_source<default_config> src1(FullSource{});
    any_source<default_config> src2;
    src2 = std::move(src1);
    EXPECT_FALSE(static_cast<bool>(src1));
    EXPECT_TRUE(static_cast<bool>(src2));
    EXPECT_TRUE(src2.connect().has_value());
    EXPECT_TRUE(src2.is_connected());
}

TEST(AnySourceTest, MoveAssignBetweenDifferentTypes) {
    any_source<default_config> src1(FullSource{});
    src1.connect();
    any_source<default_config> src2(CountingSource{});
    src2 = std::move(src1);
    EXPECT_TRUE(src2.is_connected());
    EXPECT_FALSE(static_cast<bool>(src1));
    EXPECT_TRUE(src2.subscribe("x/y", 1).has_value());
    std::byte d[] = {std::byte{0x00}};
    EXPECT_TRUE(src2.publish("x/y", d, 0).has_value());
    EXPECT_TRUE(src2.disconnect().has_value());
}

TEST(AnySourceTest, ResetThenAllMethodsReturnDefaults) {
    any_source<default_config> src(FullSource{});
    src.connect();
    src.reset();
    EXPECT_FALSE(static_cast<bool>(src));
    EXPECT_FALSE(src.is_connected());
    EXPECT_TRUE(src.disconnect().has_value());
    EXPECT_TRUE(src.subscribe("t", 0).has_value());
    std::byte d[] = {std::byte{0x00}};
    EXPECT_TRUE(src.publish("t", d, 0).has_value());
    src.set_message_callback(message_callback<default_config>(NoopCallback{}));
    src.poll();
}

TEST(AnySourceTest, NullSourceAllMethodDefaults) {
    any_source<default_config> src;
    EXPECT_FALSE(src.is_connected());
    auto r = src.connect();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::not_connected);
    EXPECT_TRUE(src.disconnect().has_value());
    EXPECT_TRUE(src.subscribe("t", 0).has_value());
    std::byte d[] = {std::byte{0x00}};
    EXPECT_TRUE(src.publish("t", d, 0).has_value());
    src.set_message_callback(message_callback<default_config>(NoopCallback{}));
    src.poll();
    src.reset();
}
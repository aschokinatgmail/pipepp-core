#include <gtest/gtest.h>
#include <pipepp/core/source_registry.hpp>
#include "helpers/test_sources.hpp"

using namespace pipepp::core;

TEST(SourceRegistryTest, EmptySize) {
    source_registry reg;
    EXPECT_EQ(reg.size(), 0u);
}

TEST(SourceRegistryTest, RegisterAndSize) {
    source_registry reg;
    EXPECT_TRUE(reg.register_scheme<MockSource>("mqtt"));
    EXPECT_EQ(reg.size(), 1u);
}

TEST(SourceRegistryTest, HasSchemeFound) {
    source_registry reg;
    reg.register_scheme<MockSource>("mqtt");
    EXPECT_TRUE(reg.has_scheme("mqtt"));
}

TEST(SourceRegistryTest, HasSchemeNotFound) {
    source_registry reg;
    reg.register_scheme<MockSource>("mqtt");
    EXPECT_FALSE(reg.has_scheme("tcp"));
}

TEST(SourceRegistryTest, CreateWithSchemeOnly) {
    source_registry reg;
    reg.register_scheme<MockSource>("mqtt");
    auto src = reg.create("mqtt");
    EXPECT_TRUE(static_cast<bool>(src));
}

TEST(SourceRegistryTest, CreateWithFullUri) {
    source_registry reg;
    reg.register_scheme<MockSource>("mqtt");
    auto src = reg.create("mqtt://broker:1883/sensors/#");
    EXPECT_TRUE(static_cast<bool>(src));
}

TEST(SourceRegistryTest, CreateUnregisteredSchemeOnly) {
    source_registry reg;
    auto src = reg.create("mqtt");
    EXPECT_FALSE(static_cast<bool>(src));
}

TEST(SourceRegistryTest, CreateUnregisteredFullUri) {
    source_registry reg;
    auto src = reg.create("mqtt://broker:1883/topic");
    EXPECT_FALSE(static_cast<bool>(src));
}

TEST(SourceRegistryTest, RegisterMultipleSchemes) {
    source_registry reg;
    EXPECT_TRUE(reg.register_scheme<MockSource>("mqtt"));
    EXPECT_TRUE(reg.register_scheme<CountingSource>("tcp"));
    EXPECT_EQ(reg.size(), 2u);

    auto mqtt_src = reg.create("mqtt://broker:1883");
    EXPECT_TRUE(static_cast<bool>(mqtt_src));

    auto tcp_src = reg.create("tcp://host:80");
    EXPECT_TRUE(static_cast<bool>(tcp_src));
}

TEST(SourceRegistryTest, RegistryFull) {
    source_registry<embedded_config> reg;
    EXPECT_TRUE(reg.register_scheme<CountingSource>("a"));
    EXPECT_TRUE(reg.register_scheme<CountingSource>("b"));
    EXPECT_FALSE(reg.register_scheme<CountingSource>("c"));
    EXPECT_EQ(reg.size(), 2u);
}

TEST(SourceRegistryTest, CreateNttpDispatch) {
    source_registry reg;
    reg.register_scheme<MockSource>("mqtt");
    auto src = reg.create<fixed_string_literal("mqtt://broker:1883/sensors/#")>();
    EXPECT_TRUE(static_cast<bool>(src));
}

TEST(SourceRegistryTest, CreateNttpUnregistered) {
    source_registry reg;
    auto src = reg.create<fixed_string_literal("tcp://host:80")>();
    EXPECT_FALSE(static_cast<bool>(src));
}

TEST(SourceRegistryTest, CreateNttpSchemeOnly) {
    source_registry reg;
    reg.register_scheme<MockSource>("mqtt");
    auto src = reg.create<fixed_string_literal("mqtt")>();
    EXPECT_TRUE(static_cast<bool>(src));
}

TEST(SourceRegistryTest, HasSchemeEmptyRegistry) {
    source_registry reg;
    EXPECT_FALSE(reg.has_scheme("anything"));
}

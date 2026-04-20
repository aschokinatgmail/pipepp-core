#include <gtest/gtest.h>
#include <pipepp/core/fixed_string.hpp>
#include <pipepp/core/error_code.hpp>

using namespace pipepp::core;

TEST(FixedStringTest, DefaultConstruction) {
    fixed_string<32> fs;
    EXPECT_TRUE(fs.empty());
    EXPECT_EQ(fs.size(), 0u);
    EXPECT_STREQ(fs.c_str(), "");
}

TEST(FixedStringTest, ConstexprConstruction) {
    fixed_string<32> fs("hello");
    EXPECT_EQ(fs.size(), 5u);
    EXPECT_STREQ(fs.c_str(), "hello");
}

TEST(FixedStringTest, StringViewConstruction) {
    fixed_string<32> fs{std::string_view("world")};
    EXPECT_EQ(fs.size(), 5u);
    EXPECT_STREQ(fs.c_str(), "world");
}

TEST(FixedStringTest, StringViewConversion) {
    fixed_string<32> fs("test");
    std::string_view sv = fs;
    EXPECT_EQ(sv, "test");
}

TEST(FixedStringTest, Equality) {
    fixed_string<32> a("abc");
    fixed_string<32> b("abc");
    fixed_string<32> c("xyz");
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
}

TEST(FixedStringTest, StringViewEquality) {
    fixed_string<32> fs("hello");
    EXPECT_TRUE(fs == std::string_view("hello"));
    EXPECT_FALSE(fs == std::string_view("world"));
}

TEST(FixedStringTest, ConstCharEquality) {
    fixed_string<32> fs("test");
    EXPECT_TRUE(fs == "test");
}

TEST(FixedStringTest, Truncation) {
    fixed_string<4> fs("hello world");
    EXPECT_EQ(fs.size(), 4u);
    EXPECT_STREQ(fs.c_str(), "hell");
}

TEST(FixedStringTest, CheckedFromSuccess) {
    fixed_string<32> fs;
    auto r = fs.checked_from(std::string_view("hello"));
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(fs.size(), 5u);
}

TEST(FixedStringTest, CheckedFromOverflow) {
    fixed_string<4> fs;
    auto r = fs.checked_from(std::string_view("hello world"));
    EXPECT_FALSE(r.has_value());
}

TEST(FixedStringTest, ConversionFromSmaller) {
    fixed_string<8> small("ab");
    fixed_string<16> large(small);
    EXPECT_EQ(large.size(), 2u);
    EXPECT_STREQ(large.c_str(), "ab");
}

TEST(FixedStringTest, CapacityDoesNotChange) {
    fixed_string<32> fs;
    EXPECT_EQ(fs.capacity(), 32u);
}

TEST(FixedStringTest, Ordering) {
    fixed_string<32> a("abc");
    fixed_string<32> b("abd");
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(a < std::string_view("abd"));
}

TEST(FixedStringTest, CheckedFromEmptyString) {
    fixed_string<32> fs;
    auto r = fs.checked_from(std::string_view(""));
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(fs.size(), 0u);
}

TEST(FixedStringTest, CheckedFromExactSize) {
    fixed_string<4> fs;
    auto r = fs.checked_from(std::string_view("abcd"));
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(fs.size(), 4u);
}

TEST(FixedStringTest, InequalityOperators) {
    fixed_string<32> a("abc");
    fixed_string<32> b("def");
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(a != std::string_view("def"));
}

TEST(FixedStringTest, SubscriptAccess) {
    fixed_string<32> fs("hello");
    EXPECT_EQ(fs[0], 'h');
    EXPECT_EQ(fs[4], 'o');
}

TEST(FixedStringLiteralTest, NonNullTerminated) {
    fixed_string_literal<3> fsl;
    fsl.data_[0] = 'a';
    fsl.data_[1] = 'b';
    fsl.data_[2] = 'c';
    EXPECT_EQ(fsl.size(), 3u);
    EXPECT_FALSE(fsl.empty());
}

TEST(FixedStringLiteralTest, NullTerminated) {
    fixed_string_literal<4> fsl;
    fsl.data_[0] = 'a';
    fsl.data_[1] = 'b';
    fsl.data_[2] = '\0';
    fsl.data_[3] = '\0';
    EXPECT_EQ(fsl.size(), 3u);
}
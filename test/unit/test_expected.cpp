#include <gtest/gtest.h>
#include <pipepp/core/expected.hpp>

using namespace pipepp::core;

TEST(ExpectedTest, DefaultResultIsSuccess) {
    result r;
    EXPECT_TRUE(r.has_value());
    EXPECT_TRUE(static_cast<bool>(r));
}

TEST(ExpectedTest, UnexpectResultIsFailure) {
    result r{unexpect, make_unexpected(error_code::not_connected)};
    EXPECT_FALSE(r.has_value());
    EXPECT_FALSE(static_cast<bool>(r));
    EXPECT_EQ(r.error(), error_code::not_connected);
}

TEST(ExpectedTest, ValueOrOnSuccess) {
    expected<int, error_code> e{42};
    EXPECT_EQ(e.value_or(0), 42);
}

TEST(ExpectedTest, ValueOrOnFailure) {
    expected<int, error_code> e{unexpect, make_unexpected(error_code::buffer_overflow)};
    EXPECT_EQ(e.value_or(99), 99);
}

TEST(ExpectedTest, ErrorMessageStrings) {
    EXPECT_STREQ(error_code_message(error_code::buffer_overflow), "buffer overflow");
    EXPECT_STREQ(error_code_message(error_code::not_connected), "not connected");
    EXPECT_STREQ(error_code_message(error_code::timeout), "timeout");
}

TEST(ExpectedTest, GenericExpectedValueAccess) {
    expected<int, error_code> e{10};
    EXPECT_EQ(*e, 10);
    EXPECT_EQ(e.value(), 10);
}

TEST(ExpectedTest, GenericExpectedMove) {
    expected<std::string, error_code> e1{"hello"};
    expected<std::string, error_code> e2 = std::move(e1);
    EXPECT_EQ(*e2, "hello");
}

TEST(ExpectedTest, MakeUnexpected) {
    auto u = make_unexpected(error_code::disconnected);
    EXPECT_EQ(u.error(), error_code::disconnected);
}

TEST(ExpectedTest, VoidExpectedMoveWithError) {
    result r1{unexpect, make_unexpected(error_code::timeout)};
    result r2 = std::move(r1);
    EXPECT_FALSE(r2.has_value());
    EXPECT_EQ(r2.error(), error_code::timeout);
}

TEST(ExpectedTest, ExpectedCopyWithError) {
    result r1{unexpect, make_unexpected(error_code::not_connected)};
    result r2 = r1;
    EXPECT_FALSE(r2.has_value());
    EXPECT_EQ(r2.error(), error_code::not_connected);
}

TEST(ExpectedTest, ExpectedMoveAssignWithError) {
    result r1{unexpect, make_unexpected(error_code::buffer_overflow)};
    result r2;
    r2 = std::move(r1);
    EXPECT_FALSE(r2.has_value());
    EXPECT_EQ(r2.error(), error_code::buffer_overflow);
}

TEST(ExpectedTest, GenericExpectedValueOrMove) {
    expected<std::string, error_code> e1{"hello"};
    auto val = std::move(e1).value_or("default");
    EXPECT_EQ(val, "hello");
}

TEST(ExpectedTest, GenericExpectedMoveAssign) {
    expected<int, error_code> e1{42};
    expected<int, error_code> e2;
    e2 = std::move(e1);
    EXPECT_TRUE(e2.has_value());
    EXPECT_EQ(*e2, 42);
}

TEST(ExpectedTest, MoveGenericExpectedWithError) {
    expected<int, error_code> e1{unexpect, make_unexpected(error_code::timeout)};
    expected<int, error_code> e2 = std::move(e1);
    EXPECT_FALSE(e2.has_value());
    EXPECT_EQ(e2.error(), error_code::timeout);
}

TEST(ExpectedTest, MoveAssignGenericExpectedWithError) {
    expected<int, error_code> e1{unexpect, make_unexpected(error_code::invalid_argument)};
    expected<int, error_code> e2{99};
    e2 = std::move(e1);
    EXPECT_FALSE(e2.has_value());
    EXPECT_EQ(e2.error(), error_code::invalid_argument);
}

TEST(ExpectedTest, CopyGenericExpectedWithError) {
    expected<int, error_code> e1{unexpect, make_unexpected(error_code::out_of_range)};
    expected<int, error_code> e2 = e1;
    EXPECT_FALSE(e2.has_value());
    EXPECT_EQ(e2.error(), error_code::out_of_range);
}
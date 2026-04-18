#include <gtest/gtest.h>
#include <pipepp/core/error_code.hpp>
#include <pipepp/core/expected.hpp>

using namespace pipepp::core;

TEST(ErrorCodeTest, AllCodesHaveMessages) {
    EXPECT_NE(error_code_message(error_code::buffer_overflow), nullptr);
    EXPECT_NE(error_code_message(error_code::not_connected), nullptr);
    EXPECT_NE(error_code_message(error_code::connection_failed), nullptr);
    EXPECT_NE(error_code_message(error_code::invalid_argument), nullptr);
    EXPECT_NE(error_code_message(error_code::out_of_range), nullptr);
    EXPECT_NE(error_code_message(error_code::capacity_exceeded), nullptr);
    EXPECT_NE(error_code_message(error_code::disconnected), nullptr);
    EXPECT_NE(error_code_message(error_code::timeout), nullptr);
    EXPECT_NE(error_code_message(error_code::stage_dropped), nullptr);
}

TEST(ErrorCodeTest, IsOkPredicate) {
    EXPECT_TRUE(is_ok(result{}));
    EXPECT_FALSE(is_ok(result{unexpect, make_unexpected(error_code::buffer_overflow)}));
    EXPECT_FALSE(is_ok(result{unexpect, make_unexpected(error_code::not_connected)}));
}

TEST(ErrorCodeTest, UnderlyingTypeIsUint8) {
    error_code ec = error_code::timeout;
    EXPECT_EQ(static_cast<uint8_t>(ec), 11);
}

TEST(ErrorCodeTest, UnknownErrorCodeReturnsUnknown) {
    error_code unknown = static_cast<error_code>(255);
    EXPECT_STREQ(error_code_message(unknown), "unknown error");
}

TEST(ErrorCodeTest, IsOkWithExplicitResult) {
    result ok_result;
    EXPECT_TRUE(is_ok(ok_result));
    result err_result{unexpect, make_unexpected(error_code::connection_failed)};
    EXPECT_FALSE(is_ok(err_result));
}

TEST(ErrorCodeTest, InvalidUriHasMessage) {
    EXPECT_NE(error_code_message(error_code::invalid_uri), nullptr);
    EXPECT_STREQ(error_code_message(error_code::invalid_uri), "invalid uri");
}
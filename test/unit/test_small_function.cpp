#include <gtest/gtest.h>
#include <pipepp/core/small_function.hpp>

using namespace pipepp::core;

TEST(SmallFunctionTest, DefaultIsNull) {
    small_function<int(int), 64> fn;
    EXPECT_FALSE(static_cast<bool>(fn));
}

TEST(SmallFunctionTest, NullptrConstruction) {
    small_function<int(int), 64> fn{nullptr};
    EXPECT_FALSE(static_cast<bool>(fn));
}

TEST(SmallFunctionTest, LambdaInvocation) {
    small_function<int(int), 64> fn([](int x) { return x * 2; });
    EXPECT_TRUE(static_cast<bool>(fn));
    EXPECT_EQ(fn(21), 42);
}

TEST(SmallFunctionTest, ConstInvocation) {
    const small_function<int(int), 64> fn([](int x) { return x + 1; });
    EXPECT_EQ(fn(9), 10);
}

TEST(SmallFunctionTest, MutableLambda) {
    int counter = 0;
    small_function<int(), 64> fn([&counter]() mutable { return ++counter; });
    EXPECT_EQ(fn(), 1);
    EXPECT_EQ(fn(), 2);
    EXPECT_EQ(fn(), 3);
}

TEST(SmallFunctionTest, MoveConstruction) {
    int val = 10;
    small_function<int(int), 64> fn1([val](int x) { return x + val; });
    small_function<int(int), 64> fn2 = std::move(fn1);
    EXPECT_FALSE(static_cast<bool>(fn1));
    EXPECT_TRUE(static_cast<bool>(fn2));
    EXPECT_EQ(fn2(5), 15);
}

TEST(SmallFunctionTest, MoveAssignment) {
    small_function<int(int), 64> fn1([](int x) { return x * 3; });
    small_function<int(int), 64> fn2;
    fn2 = std::move(fn1);
    EXPECT_FALSE(static_cast<bool>(fn1));
    EXPECT_EQ(fn2(7), 21);
}

TEST(SmallFunctionTest, Reset) {
    small_function<void(), 64> fn([]() {});
    EXPECT_TRUE(static_cast<bool>(fn));
    fn.reset();
    EXPECT_FALSE(static_cast<bool>(fn));
}

TEST(SmallFunctionTest, CapturingLambda) {
    std::string value = "hello";
    small_function<std::string_view(), 64> fn([value]() { return std::string_view(value); });
    EXPECT_EQ(fn(), "hello");
}

TEST(SmallFunctionTest, SizeAssertionOnTooLarge) {
    // This should fail at compile time if uncommented:
    // small_function<int(), 1> fn([](){ return 0; }); // too large
    // Just verify normal sizes work
    small_function<int(), 256> fn([]() { return 42; });
    EXPECT_EQ(fn(), 42);
}

TEST(SmallFunctionTest, VoidReturnType) {
    bool called = false;
    small_function<void(), 64> fn([&called]() { called = true; });
    fn();
    EXPECT_TRUE(called);
}

TEST(SmallFunctionTest, EmbeddedConfigSize) {
    small_function<int(int), 128> fn([](int x) { return x; });
    EXPECT_EQ(fn(42), 42);
}
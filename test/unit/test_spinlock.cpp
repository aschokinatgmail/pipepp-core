#include <gtest/gtest.h>
#include <pipepp/core/spinlock.hpp>
#include <thread>
#include <atomic>

using namespace pipepp::core;

TEST(SpinlockTest, LockAndUnlock) {
    spinlock lk;
    lk.lock();
    EXPECT_FALSE(lk.try_lock());
    lk.unlock();
    EXPECT_TRUE(lk.try_lock());
    lk.unlock();
}

TEST(SpinlockTest, TryLockSucceedsWhenUnlocked) {
    spinlock lk;
    EXPECT_TRUE(lk.try_lock());
    lk.unlock();
}

TEST(SpinlockTest, TryLockFailsWhenLocked) {
    spinlock lk;
    lk.lock();
    EXPECT_FALSE(lk.try_lock());
    lk.unlock();
}

TEST(SpinlockTest, GuardLocksAndUnlocks) {
    spinlock lk;
    {
        spinlock_guard guard(lk);
        EXPECT_FALSE(lk.try_lock());
    }
    EXPECT_TRUE(lk.try_lock());
    lk.unlock();
}

TEST(SpinlockTest, MoveFromUnlocked) {
    spinlock lk1;
    spinlock lk2 = std::move(lk1);
    EXPECT_TRUE(lk2.try_lock());
    lk2.unlock();
}

TEST(SpinlockTest, ConcurrentLockUnlock) {
    spinlock lk;
    std::atomic<int> counter{0};
    constexpr int iterations = 1000;

    auto worker = [&]() {
        for (int i = 0; i < iterations; ++i) {
            spinlock_guard guard(lk);
            ++counter;
        }
    };

    std::thread t1(worker);
    std::thread t2(worker);
    t1.join();
    t2.join();

    EXPECT_EQ(counter.load(), 2 * iterations);
}

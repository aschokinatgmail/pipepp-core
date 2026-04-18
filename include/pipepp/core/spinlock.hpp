#pragma once

#include <atomic>

namespace pipepp::core {

class spinlock {
public:
    spinlock() = default;

    spinlock(const spinlock&) = delete;
    spinlock& operator=(const spinlock&) = delete;

    spinlock(spinlock&&) noexcept {}
    spinlock& operator=(spinlock&&) noexcept { return *this; }

    void lock() {
        while (locked_.exchange(true, std::memory_order_acquire)) {}
    }

    void unlock() {
        locked_.store(false, std::memory_order_release);
    }

    bool try_lock() {
        return !locked_.exchange(true, std::memory_order_acquire);
    }

private:
    std::atomic<bool> locked_{false};
};

class spinlock_guard {
public:
    explicit spinlock_guard(spinlock& lk) : lk_(lk) { lk_.lock(); }
    ~spinlock_guard() { lk_.unlock(); }

    spinlock_guard(const spinlock_guard&) = delete;
    spinlock_guard& operator=(const spinlock_guard&) = delete;

private:
    spinlock& lk_;
};

} // namespace pipepp::core
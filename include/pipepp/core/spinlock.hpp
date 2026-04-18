#pragma once

#include <atomic>
#include <cassert>

namespace pipepp::core {

class spinlock {
public:
    spinlock() = default;

    spinlock(const spinlock&) = delete;
    spinlock& operator=(const spinlock&) = delete;

    spinlock(spinlock&& other) noexcept {
        assert(!other.locked_.load(std::memory_order_acquire) && "moving a locked spinlock");
    }
    spinlock& operator=(spinlock&& other) noexcept {
        assert(!other.locked_.load(std::memory_order_acquire) && "moving a locked spinlock");
        return *this;
    }

    void lock() {
        while (locked_.exchange(true, std::memory_order_acquire)) {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
            __builtin_ia32_pause();
#else
            std::atomic_thread_fence(std::memory_order_relaxed);
#endif
        }
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

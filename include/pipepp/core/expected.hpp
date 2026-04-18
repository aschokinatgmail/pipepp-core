#pragma once

#include <cstddef>
#include <type_traits>
#include <new>
#include <utility>

#include "error_code.hpp"

namespace pipepp::core {

struct unexpect_t {};
inline constexpr unexpect_t unexpect{};

template<typename E>
class unexpected {
public:
    constexpr unexpected(E e) : error_(std::move(e)) {}
    constexpr const E& error() const noexcept { return error_; }
    constexpr E& error() noexcept { return error_; }
private:
    E error_;
};

namespace detail {

struct uninitialized_t {};
inline constexpr uninitialized_t uninitialized{};

}

template<typename T, typename E = error_code>
class expected {
public:
    constexpr expected() : has_value_(true) { ::new (&val_) T(); }
    constexpr expected(T v) : has_value_(true) { ::new (&val_) T(std::move(v)); }
    constexpr expected(unexpect_t, unexpected<E> u) : has_value_(false) { ::new (&err_) E(std::move(u).error()); }

    expected(const expected& o) : has_value_(o.has_value_) {
        if (has_value_) ::new (&val_) T(o.val_);
        else            ::new (&err_) E(o.err_);
    }

    expected(expected&& o) noexcept(std::is_nothrow_move_constructible_v<T>
                                     && std::is_nothrow_move_constructible_v<E>)
        : has_value_(o.has_value_) {
        if (has_value_) ::new (&val_) T(std::move(o.val_));
        else            ::new (&err_) E(std::move(o.err_));
    }

    expected& operator=(const expected& o) {
        if (this != &o) {
            destroy();
            has_value_ = o.has_value_;
            if (has_value_) ::new (&val_) T(o.val_);
            else            ::new (&err_) E(o.err_);
        }
        return *this;
    }

    expected& operator=(expected&& o) noexcept(std::is_nothrow_move_assignable_v<T>
                                                && std::is_nothrow_move_assignable_v<E>) {
        if (this != &o) {
            destroy();
            has_value_ = o.has_value_;
            if (has_value_) ::new (&val_) T(std::move(o.val_));
            else            ::new (&err_) E(std::move(o.err_));
        }
        return *this;
    }

    ~expected() { destroy(); }

    constexpr bool has_value() const noexcept { return has_value_; }
    constexpr explicit operator bool() const noexcept { return has_value_; }

    constexpr T& value() & { return val_; }
    constexpr const T& value() const& { return val_; }
    constexpr T&& value() && { return std::move(val_); }

    constexpr T& operator*() & { return val_; }
    constexpr const T& operator*() const& { return val_; }
    constexpr T&& operator*() && { return std::move(val_); }

    constexpr E& error() & { return err_; }
    constexpr const E& error() const& { return err_; }

    template<typename U>
    constexpr T value_or(U&& default_val) const& {
        return has_value_ ? val_ : static_cast<T>(std::forward<U>(default_val));
    }

    template<typename U>
    constexpr T value_or(U&& default_val) && {
        return has_value_ ? std::move(val_) : static_cast<T>(std::forward<U>(default_val));
    }

private:
    void destroy() {
        if (has_value_) val_.~T();
        else            err_.~E();
    }

    bool has_value_;
    union {
        T val_;
        E err_;
    };
};

template<typename E>
class expected<void, E> {
public:
    constexpr expected() : has_value_(true) {}
    constexpr expected(unexpect_t, unexpected<E> u) : has_value_(false) {
        ::new (&err_) E(std::move(u).error());
    }

    expected(const expected& o) : has_value_(o.has_value_) {
        if (!has_value_) ::new (&err_) E(o.err_);
    }

    expected(expected&& o) noexcept(std::is_nothrow_move_constructible_v<E>)
        : has_value_(o.has_value_) {
        if (!has_value_) ::new (&err_) E(std::move(o.err_));
    }

    expected& operator=(const expected& o) {
        if (this != &o) {
            destroy();
            has_value_ = o.has_value_;
            if (!has_value_) ::new (&err_) E(o.err_);
        }
        return *this;
    }

    expected& operator=(expected&& o) noexcept(std::is_nothrow_move_assignable_v<E>) {
        if (this != &o) {
            destroy();
            has_value_ = o.has_value_;
            if (!has_value_) ::new (&err_) E(std::move(o.err_));
        }
        return *this;
    }

    ~expected() { destroy(); }

    constexpr bool has_value() const noexcept { return has_value_; }
    constexpr explicit operator bool() const noexcept { return has_value_; }

    constexpr E& error() & { return err_; }
    constexpr const E& error() const& { return err_; }

    void value() const noexcept {}

private:
    void destroy() {
        if (!has_value_) err_.~E();
    }

    bool has_value_;
    union { E err_; };
};

using result = expected<void, error_code>;

inline bool is_ok(result r) noexcept {
    return r.has_value();
}

template<typename E>
constexpr unexpected<E> make_unexpected(E e) {
    return unexpected<E>(std::move(e));
}

} // namespace pipepp::core
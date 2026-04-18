#pragma once

#include <cstddef>
#include <cstring>
#include <type_traits>
#include <new>
#include <utility>

namespace pipepp::core {

template<std::size_t N>
class small_function_storage {
public:
    constexpr small_function_storage() = default;

    void* data() noexcept { return buf_; }
    const void* data() const noexcept { return buf_; }
    constexpr std::size_t size() const noexcept { return N; }

private:
    alignas(std::max_align_t) std::byte buf_[N];
};

template<typename Sig, std::size_t N>
class small_function;

template<typename R, typename... Args, std::size_t N>
class small_function<R(Args...), N> {
public:
    using result_type = R;

    constexpr small_function() noexcept = default;
    constexpr small_function(std::nullptr_t) noexcept : small_function() {}

    small_function(const small_function&) = delete;
    small_function& operator=(const small_function&) = delete;

    small_function(small_function&& other) noexcept
        : invoke_fn_(other.invoke_fn_)
        , const_destroy_fn_(other.const_destroy_fn_)
        , move_fn_(other.move_fn_) {
        if (move_fn_) {
            move_fn_(other.storage_.data(), storage_.data());
            const_destroy_fn_(other.storage_.data());
        }
        other.invoke_fn_ = nullptr;
        other.const_destroy_fn_ = nullptr;
        other.move_fn_ = nullptr;
    }

    small_function& operator=(small_function&& other) noexcept {
        if (this != &other) {
            reset();
            invoke_fn_ = other.invoke_fn_;
            const_destroy_fn_ = other.const_destroy_fn_;
            move_fn_ = other.move_fn_;
            if (move_fn_) {
                move_fn_(other.storage_.data(), storage_.data());
                const_destroy_fn_(other.storage_.data());
            }
            other.invoke_fn_ = nullptr;
            other.const_destroy_fn_ = nullptr;
            other.move_fn_ = nullptr;
        }
        return *this;
    }

    ~small_function() { reset(); }

    template<typename F, typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, small_function>>>
    small_function(F&& f) {
        using Decayed = std::decay_t<F>;
        static_assert(sizeof(Decayed) <= N,
                      "Callable does not fit in small_function buffer");
        static_assert(std::is_nothrow_move_constructible_v<Decayed>,
                      "Callable must be nothrow move constructible");

        ::new (storage_.data()) Decayed(std::forward<F>(f));
        invoke_fn_ = &invoke_impl<Decayed>;
        const_destroy_fn_ = &destroy_impl<Decayed>;
        move_fn_ = &move_impl<Decayed>;
    }

    R operator()(Args... args) const {
        return invoke_fn_(storage_.data(), std::forward<Args>(args)...);
    }

    explicit operator bool() const noexcept { return invoke_fn_ != nullptr; }

    void reset() noexcept {
        if (const_destroy_fn_) {
            const_destroy_fn_(storage_.data());
        }
        invoke_fn_ = nullptr;
        const_destroy_fn_ = nullptr;
        move_fn_ = nullptr;
    }

private:
    template<typename Decayed>
    static R invoke_impl(const void* obj, Args... args) {
        return (*const_cast<Decayed*>(static_cast<const Decayed*>(obj)))(std::forward<Args>(args)...);
    }

    template<typename Decayed>
    static void destroy_impl(void* obj) {
        static_cast<Decayed*>(obj)->~Decayed();
    }

    template<typename Decayed>
    static void move_impl(void* src, void* dst) noexcept {
        ::new (dst) Decayed(std::move(*static_cast<Decayed*>(src)));
    }

    using invoke_fn_t = R(*)(const void*, Args...);
    using destroy_fn_t = void(*)(void*);
    using move_fn_t = void(*)(void*, void*) noexcept;

    invoke_fn_t invoke_fn_ = nullptr;
    destroy_fn_t const_destroy_fn_ = nullptr;
    move_fn_t move_fn_ = nullptr;
    small_function_storage<N> storage_;
};

} // namespace pipepp::core
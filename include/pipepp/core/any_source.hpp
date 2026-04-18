#pragma once

#include <atomic>
#include <cstddef>
#include <cstring>
#include <string_view>
#include <new>
#include <utility>

#include "concepts.hpp"
#include "config.hpp"
#include "error_code.hpp"
#include "expected.hpp"
#include "message.hpp"
#include "message_callback.hpp"

namespace pipepp::core {

template<typename Config = default_config>
class any_source {
public:
    using callback_type = message_callback<Config>;

    any_source() noexcept : vtable_(nullptr) {}

    any_source(const any_source&) = delete;
    any_source& operator=(const any_source&) = delete;

    any_source(any_source&& other) noexcept
        : vtable_(other.vtable_) {
        if (vtable_ && vtable_->move_fn) {
            vtable_->move_fn(other.storage_.data(), storage_.data());
            vtable_->destroy_fn(other.storage_.data());
        }
        other.vtable_ = nullptr;
    }

    any_source& operator=(any_source&& other) noexcept {
        if (this != &other) {
            reset();
            vtable_ = other.vtable_;
            if (vtable_ && vtable_->move_fn) {
                vtable_->move_fn(other.storage_.data(), storage_.data());
                vtable_->destroy_fn(other.storage_.data());
            }
            other.vtable_ = nullptr;
        }
        return *this;
    }

    ~any_source() { reset(); }

    template<typename Source>
        requires BusSource<std::decay_t<Source>, Config>
    any_source(Source&& src) {
        using Decayed = std::decay_t<Source>;
        static_assert(sizeof(Decayed) <= Config::source_size,
                      "Source type does not fit in any_source storage");
        static_assert(std::is_nothrow_move_constructible<Decayed>::value,
                      "Source must be nothrow move constructible");

        vtable_ = vtable_for<Decayed>();
        ::new (storage_.data()) Decayed(std::forward<Source>(src));
    }

    result connect(uri_view uri = {}) {
        return vtable_ ? vtable_->connect_fn(storage_.data(), uri) 
                       : result{unexpect, make_unexpected(error_code::not_connected)};
    }

    result disconnect() {
        return vtable_ ? vtable_->disconnect_fn(storage_.data()) : result{};
    }

    bool is_connected() const {
        return vtable_ ? vtable_->is_connected_fn(storage_.data()) : false;
    }

    result subscribe(std::string_view topic, int qos) {
        return vtable_ ? vtable_->subscribe_fn(storage_.data(), topic, qos) : result{};
    }

    result publish(std::string_view topic, std::span<const std::byte> payload, int qos) {
        return vtable_ ? vtable_->publish_fn(storage_.data(), topic, payload, qos) : result{};
    }

    void set_message_callback(callback_type cb) {
        if (vtable_) {
            vtable_->set_callback_fn(storage_.data(), std::move(cb));
        }
    }

    void poll() {
        if (vtable_) {
            vtable_->poll_fn(storage_.data());
        }
    }

    explicit operator bool() const noexcept { return vtable_ != nullptr; }

    void reset() noexcept {
        if (vtable_ && vtable_->destroy_fn) {
            vtable_->destroy_fn(storage_.data());
        }
        vtable_ = nullptr;
    }

private:
    struct vtable {
        result (*connect_fn)(void*, uri_view);
        result (*disconnect_fn)(void*);
        bool (*is_connected_fn)(const void*);
        result (*subscribe_fn)(void*, std::string_view, int);
        result (*publish_fn)(void*, std::string_view, std::span<const std::byte>, int);
        void (*set_callback_fn)(void*, callback_type);
        void (*poll_fn)(void*);
        void (*destroy_fn)(void*);
        void (*move_fn)(void*, void*);
    };

    template<typename Source>
    static const vtable* vtable_for() {
        static const vtable vt = {
            &connect_impl<Source>,
            &disconnect_impl<Source>,
            &is_connected_impl<Source>,
            &subscribe_impl<Source>,
            &publish_impl<Source>,
            &set_callback_impl<Source>,
            &poll_impl<Source>,
            &destroy_impl<Source>,
            &move_impl<Source>
        };
        return &vt;
    }

    template<typename Source>
    static result connect_impl(void* self, uri_view uri) {
        return static_cast<Source*>(self)->connect(uri);
    }

    template<typename Source>
    static result disconnect_impl(void* self) {
        return static_cast<Source*>(self)->disconnect();
    }

    template<typename Source>
    static bool is_connected_impl(const void* self) {
        return static_cast<const Source*>(self)->is_connected();
    }

    template<typename Source>
    static result subscribe_impl(void* self, std::string_view topic, int qos) {
        return static_cast<Source*>(self)->subscribe(topic, qos);
    }

    template<typename Source>
    static result publish_impl(void* self, std::string_view topic,
                                std::span<const std::byte> payload, int qos) {
        return static_cast<Source*>(self)->publish(topic, payload, qos);
    }

    template<typename Source>
    static void set_callback_impl(void* self, callback_type cb) {
        static_cast<Source*>(self)->set_message_callback(std::move(cb));
    }

    template<typename Source>
    static void poll_impl(void* self) {
        static_cast<Source*>(self)->poll();
    }

    template<typename Source>
    static void destroy_impl(void* self) {
        static_cast<Source*>(self)->~Source();
    }

    template<typename Source>
    static void move_impl(void* src, void* dst) {
        ::new (dst) Source(std::move(*static_cast<Source*>(src)));
    }

    const vtable* vtable_;
    small_function_storage<Config::source_size> storage_;
};

} // namespace pipepp::core
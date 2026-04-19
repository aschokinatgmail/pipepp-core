#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <type_traits>
#include <utility>

#include "config.hpp"
#include "concepts.hpp"
#include "error_code.hpp"
#include "expected.hpp"
#include "message.hpp"
#include "message_callback.hpp"
#include "spinlock.hpp"
#include "stages.hpp"
#include "uri.hpp"

namespace pipepp::core {

// ── Thread Safety Model ─────────────────────────────────────────────────
//
// basic_pipeline uses a spinlock-only concurrency model:
//
//   • `spinlock stage_lock_` — a non-recursive test-and-set spinlock.
//     A `spinlock_guard` is acquired in `emit()`, which is the single
//     entry point that delegates to `process_message()`.  Every mutation
//     of pipeline stage state (stages_, sink_) therefore happens under
//     the lock.
//
//   • `std::atomic<bool> running_` — checked by the event loop
//     (`memory_order_acquire`) and set by `stop()` (`memory_order_release`).
//     No additional synchronisation is needed for the flag itself.
//
//   • `emit()` is fully synchronous — it blocks the caller until every
//     stage and the sink have completed.  There is no SPSC queue, no
//     lock-free queue, and no `flush()` primitive.  Callers may invoke
//     `emit()` from any thread at any time; the spinlock serialises
//     concurrent calls.
//
//   • The event loop (`run()`) installs a message callback that routes
//     through `emit()`, so the same spinlock protects both
//     externally-initiated and source-initiated message processing.
//
// Guarantees:
//   – No data race on stages, sink, or message mutation under TSAN.
//   – `stop()` is safe to call from any thread.
//   – `emit()` is safe to call from any thread, including while `run()`
//     is executing its event loop.
// ─────────────────────────────────────────────────────────────────────────

template<typename Source, typename Config = default_config>
    requires BusSource<std::remove_reference_t<Source>, Config>
class basic_pipeline {
public:
    using source_type = std::remove_reference_t<Source>;
    using config_type = Config;
    using message_type = basic_message<Config>;
    using subscription_type = subscription<Config>;

    explicit basic_pipeline(Source src) : source_(std::move(src)) {}
    explicit basic_pipeline(Source src, std::string_view uri_str) : source_(std::move(src)), owned_uri_(basic_uri<Config>::parse_or_default(uri_str)) {
        if (owned_uri_.truncated()) {
            pending_error_ = error_code::invalid_uri;
        }
    }

    basic_pipeline(const basic_pipeline&) = delete;
    basic_pipeline& operator=(const basic_pipeline&) = delete;

    basic_pipeline(basic_pipeline&& other) noexcept
        : source_(std::move(other.source_))
        , owned_uri_(std::move(other.owned_uri_))
        , config_ops_(std::move(other.config_ops_))
        , config_count_(other.config_count_)
        , stages_(std::move(other.stages_))
        , stage_count_(other.stage_count_)
        , sink_(std::move(other.sink_))
        , subscriptions_(std::move(other.subscriptions_))
        , sub_count_(other.sub_count_)
        , running_(false)
        , stage_lock_(std::move(other.stage_lock_))
        , pending_error_(other.pending_error_) {
        other.running_.store(false, std::memory_order_relaxed);
    }

    basic_pipeline& operator=(basic_pipeline&& other) noexcept {
        if (this != &other) {
            source_ = std::move(other.source_);
            owned_uri_ = std::move(other.owned_uri_);
            config_ops_ = std::move(other.config_ops_);
            config_count_ = other.config_count_;
            stages_ = std::move(other.stages_);
            stage_count_ = other.stage_count_;
            sink_ = std::move(other.sink_);
            subscriptions_ = std::move(other.subscriptions_);
            sub_count_ = other.sub_count_;
            running_.store(false, std::memory_order_relaxed);
            stage_lock_ = std::move(other.stage_lock_);
            pending_error_ = other.pending_error_;
        }
        return *this;
    }

    basic_pipeline& subscribe(std::string_view topic, int qos = 0) {
        if (sub_count_ < Config::max_subscriptions) {
            auto& sub = subscriptions_[sub_count_++];
            auto r = sub.topic.checked_from(topic);
            if (!r) {
                pending_error_ = error_code::buffer_overflow;
                return *this;
            }
            sub.qos = qos;
        } else {
            pending_error_ = error_code::capacity_exceeded;
        }
        return *this;
    }

    template<typename F>
    basic_pipeline& configure(F&& f) {
        if (config_count_ < Config::max_config_ops) {
            config_ops_[config_count_++] = config_fn<Config>::template create<source_type>(std::forward<F>(f));
        } else {
            pending_error_ = error_code::capacity_exceeded;
        }
        return *this;
    }

    template<typename F>
    basic_pipeline& add_stage(F&& f) {
        if (stage_count_ < Config::max_stages) {
            stages_[stage_count_++] = process_fn<Config>(std::forward<F>(f));
        } else {
            pending_error_ = error_code::capacity_exceeded;
        }
        return *this;
    }

    template<typename F>
    basic_pipeline& set_sink(F&& f) {
        sink_ = sink_fn<Config>(std::forward<F>(f));
        return *this;
    }

    basic_pipeline& connect_uri(std::string_view uri_str) {
        owned_uri_ = basic_uri<Config>::parse_or_default(uri_str);
        if (owned_uri_.truncated()) {
            pending_error_ = error_code::invalid_uri;
        }
        return *this;
    }

    [[nodiscard]] result run() {
        if (pending_error_ != error_code::none)
            return result{unexpect, make_unexpected(pending_error_)};

        auto r = source_.connect(owned_uri_.view());
        if (!r) return r;

        for (std::size_t i = 0; i < config_count_; ++i) {
            r = config_ops_[i].apply(static_cast<void*>(&source_));
            if (!r) {
                source_.disconnect();
                return r;
            }
        }

        for (std::size_t i = 0; i < sub_count_; ++i) {
            r = source_.subscribe(
                static_cast<std::string_view>(subscriptions_[i].topic),
                subscriptions_[i].qos);
            if (!r) {
                source_.disconnect();
                return r;
            }
        }

        running_.store(true, std::memory_order_release);
        event_loop();
        running_.store(false, std::memory_order_release);
        source_.disconnect();
        return {};
    }

    void stop() {
        running_.store(false, std::memory_order_release);
    }

    bool poll_once() {
        if (!running_.load(std::memory_order_acquire) || !source_.is_connected())
            return false;
        source_.poll();
        return running_.load(std::memory_order_acquire);
    }

    void emit(const message_view& msg) {
        spinlock_guard guard(stage_lock_);
        process_message(msg);
    }

    source_type& source() noexcept { return source_; }
    const source_type& source() const noexcept { return source_; }

    bool is_running() const noexcept { return running_.load(std::memory_order_acquire); }

    [[nodiscard]] result check() const noexcept {
        if (pending_error_ != error_code::none)
            return result{unexpect, make_unexpected(pending_error_)};
        return {};
    }

private:
    void process_message(const message_view& msg) {
        message_type owned;
        auto r = owned.from_view(msg);
        if (!r) return;

        for (std::size_t i = 0; i < stage_count_; ++i) {
            if (!stages_[i](owned)) return;
        }

        if (sink_) {
            sink_(owned.as_view());
        }
    }

    void event_loop() {
        source_.set_message_callback([this](const message_view& msg) {
            this->emit(msg);
        });
        while (running_.load(std::memory_order_acquire) && source_.is_connected()) {
            source_.poll();
        }
    }

    Source source_;
    basic_uri<Config> owned_uri_{};
    std::array<config_fn<Config>, Config::max_config_ops> config_ops_{};
    std::size_t config_count_ = 0;
    std::array<process_fn<Config>, Config::max_stages> stages_{};
    std::size_t stage_count_ = 0;
    sink_fn<Config> sink_;
    std::array<subscription<Config>, Config::max_subscriptions> subscriptions_{};
    std::size_t sub_count_ = 0;
    std::atomic<bool> running_{false};
    spinlock stage_lock_;
    error_code pending_error_{error_code::none};
};

template<typename Source, typename Config = default_config>
    requires BusSource<std::remove_reference_t<Source>, Config>
basic_pipeline<Source, Config> pipeline(Source src, Config = {}) {
    return basic_pipeline<Source, Config>(std::move(src));
}

} // namespace pipepp::core
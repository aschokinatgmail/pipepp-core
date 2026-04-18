#pragma once

#include <type_traits>
#include <utility>

#include "fixed_string.hpp"
#include "pipeline.hpp"

namespace pipepp::core {

template<typename F>
struct process_args {
    F fn;
};

template<typename F>
process_args<std::decay_t<F>> process(F&& f) {
    return process_args<std::decay_t<F>>{std::forward<F>(f)};
}

template<typename F>
auto transform(F&& f) {
    return process(std::forward<F>(f));
}

template<typename F>
struct filter_args {
    F fn;
};

template<typename F>
filter_args<std::decay_t<F>> filter(F&& f) {
    return filter_args<std::decay_t<F>>{std::forward<F>(f)};
}

template<typename F>
struct sink_args {
    F fn;
};

template<typename F>
sink_args<std::decay_t<F>> sink(F&& f) {
    return sink_args<std::decay_t<F>>{std::forward<F>(f)};
}

template<typename F>
struct configure_args {
    F fn;
};

template<typename F>
configure_args<std::decay_t<F>> configure(F&& f) {
    return configure_args<std::decay_t<F>>{std::forward<F>(f)};
}

template<std::size_t N>
struct subscribe_args {
    fixed_string<N> topic;
    int qos = 0;
};

template<std::size_t N>
subscribe_args<N - 1> subscribe(const char (&topic)[N], int qos = 0) {
    subscribe_args<N - 1> args;
    args.topic = fixed_string<N - 1>(topic);
    args.qos = qos;
    return args;
}

template<typename Source, typename Config, typename F>
basic_pipeline<Source, Config> operator|(basic_pipeline<Source, Config> pipe, process_args<F>&& args) {
    pipe.add_stage([fn = std::move(args.fn)](basic_message<Config>& msg) mutable -> bool {
        auto result = fn(msg);
        return static_cast<bool>(result);
    });
    return pipe;
}

template<typename Source, typename Config, typename F>
basic_pipeline<Source, Config> operator|(basic_pipeline<Source, Config> pipe, filter_args<F>&& args) {
    pipe.add_stage([fn = std::move(args.fn)](basic_message<Config>& msg) mutable -> bool {
        return fn(msg);
    });
    return pipe;
}

template<typename Source, typename Config, typename F>
basic_pipeline<Source, Config> operator|(basic_pipeline<Source, Config> pipe, sink_args<F>&& args) {
    pipe.set_sink([fn = std::move(args.fn)](const message_view& msg) mutable {
        fn(msg);
    });
    return pipe;
}

template<typename Source, typename Config, typename F>
basic_pipeline<Source, Config> operator|(basic_pipeline<Source, Config> pipe, configure_args<F>&& args) {
    pipe.configure(std::move(args.fn));
    return pipe;
}

template<typename Source, typename Config, std::size_t N>
basic_pipeline<Source, Config> operator|(basic_pipeline<Source, Config> pipe, subscribe_args<N>&& args) {
    pipe.subscribe(static_cast<std::string_view>(args.topic), args.qos);
    return pipe;
}

template<typename Source, typename Config>
result execute(basic_pipeline<Source, Config>& pipe) {
    return pipe.run();
}

} // namespace pipepp::core
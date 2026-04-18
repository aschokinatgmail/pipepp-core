#pragma once

#include <utility>

#include "pipeline.hpp"

namespace pipepp::core {

template<typename Source, typename Config, typename F>
basic_pipeline<Source, Config> with_remap(basic_pipeline<Source, Config> pipe,
                                            basic_pipeline<Source, Config>* target,
                                            F&& remap) {
    pipe.set_sink([target, remap = std::decay_t<F>(std::forward<F>(remap))](const message_view& mv) mutable {
        auto remapped_topic = remap(mv.topic);
        if (!remapped_topic.empty()) {
            message_view remapped_mv(remapped_topic, mv.payload, mv.qos);
            target->emit(remapped_mv);
        } else {
            target->emit(mv);
        }
    });
    return pipe;
}

template<typename Source, typename Config>
struct connect_args {
    basic_pipeline<Source, Config>* target;
};

/**
 * Create a connection argument for piping into another pipeline.
 *
 * WARNING: Self-referencing deadlock — connecting a pipeline to itself
 * (e.g. `pipe | connect_to(pipe)`) is undefined behaviour.  The internal
 * spinlock is non-recursive; emitting from within the sink callback will
 * deadlock.  Never pass the same pipeline instance as both source and
 * target.
 */
template<typename Source, typename Config>
connect_args<Source, Config> connect_to(basic_pipeline<Source, Config>& target) {
    return connect_args<Source, Config>{&target};
}

template<typename Source, typename Config>
basic_pipeline<Source, Config> operator|(basic_pipeline<Source, Config> pipe,
                                           connect_args<Source, Config>&& args) {
    auto* target = args.target;
    pipe.set_sink([target](const message_view& mv) {
        target->emit(mv);
    });
    return pipe;
}

} // namespace pipepp::core
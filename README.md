# pipe++

A modular C++20 pipeline library for pub-sub messaging systems. Compose data pipelines using the pipe operator (`|`), with full interoperability with `std::views` ranges.

## Design Principles

- **No RTTI** — compiles with `-fno-rtti`; no `dynamic_cast`, no `typeid`
- **No virtual** — uses C++20 concepts instead of virtual base classes; manual vtables (function pointers in `.rodata`) for type erasure
- **No dynamic memory** — all storage is fixed-size (`std::array`, `fixed_string`); small-function optimization for callables; no `std::vector` or `std::string`
- **No exceptions in public headers** — fallible operations return `expected<void, error_code>`; exceptions caught at module boundaries
- **C++20** — concepts, `std::span`, `std::string_view`, `consteval`
- **Embedded-compatible** — works with `-fno-rtti -fno-exceptions -Os`
- **Extensible at compile time** — adding a new module requires no changes to core or other modules

## Quick Start

```cpp
#include <pipepp/pipepp.hpp>

using namespace pipepp::core;

// Define a source
struct MySource {
    // BusSource concept requirements
    expected<void, error_code> connect(uri_view = {}) { return {}; }
    expected<void, error_code> disconnect() { return {}; }
    void set_message_callback(message_callback<default_config>) {}
    expected<void, error_code> poll() { return {}; }
    void publish(const message_view&) {}
};

// Create a pipeline and compose with pipe syntax
auto pipe = basic_pipeline<MySource>(MySource{})
    | transform([](basic_message<default_config>& msg) -> int { return msg.payload.size(); })
    | filter([](int size) { return size > 0; })
    | sink([](const message_view& mv) { /* handle result */ });
```

## Modules

| Module | Description | Status |
|--------|-------------|--------|
| `pipepp-core` | Generic pipeline framework (header-only) | Complete |
| `pipepp-mqtt` | MQTT specialization (depends on Eclipse Paho C++) | Planned |

## Architecture

### Pipeline Elements

Every pipeline operation falls into one of three categories:

- **Configuration** — modifies the source (e.g., `configure(fn)`)
- **Processing** — transforms or filters messages (`transform()`, `filter()`)
- **Terminal** — consumes output (`sink()`, `subscribe()`)

### Pipe Syntax

Every `.method()` has a `| stage` equivalent:

```cpp
// Method syntax
pipe.transform(fn).filter(pred).sink(consumer);

// Pipe syntax (identical behavior)
pipe | transform(fn) | filter(pred) | sink(consumer);

// Free function equivalence
execute(pipe);  // same as pipe.run()
```

### Cross-Pipeline Connection

Connect two pipelines and optionally remap topics:

```cpp
auto source_pipe = basic_pipeline<MySource>(MySource{});
auto target_pipe = basic_pipeline<MySource>(MySource{});

// Direct connection
auto connected = std::move(source_pipe) | connect_to(target_pipe);

// With topic remapping
auto remapped = with_remap(std::move(source_pipe), &target_pipe,
    [](std::string_view topic) -> std::string_view {
        return "new/topic";
    });
```

### Thread Safety

Thread safety uses a **spinlock-only** approach:
- `spinlock_guard` around `emit()` and `process_message()`
- `std::atomic<bool>` for the `running_` flag
- `emit()` is synchronous — blocks until all stages + sink complete
- No SPSC queue, no lock-free queue, no `flush()`

### URI Support

Compile-time selectable URI parser via Config:

```cpp
// Minimal parser (default)
struct my_config : default_config {
    using uri_parser = minimal_uri_tag;  // scheme://host:port/path?query#fragment
};

// Full RFC 3986 parser (IPv6, userinfo, percent-encoding)
struct my_config : default_config {
    using uri_parser = rfc3986_uri_tag;
};
```

### Source Registry

Create sources by scheme string or full URI:

```cpp
source_registry<default_config> registry;
registry.register_scheme<MqttSource>("mqtt");
registry.register_schema<MqttSource>("mqtt");

auto src = registry.create("mqtt");                    // plain scheme
auto src = registry.create("mqtt://broker:1883/topic");  // full URI
```

## Configuration

Customize buffer sizes and behavior via a Config struct:

```cpp
struct embedded_config {
    static constexpr std::size_t small_fn_size = 16;
    static constexpr std::size_t max_stages = 4;
    static constexpr std::size_t max_topic_len = 32;
    static constexpr std::size_t max_payload_len = 64;
    static constexpr std::size_t max_uri_len = 64;
    static constexpr std::size_t max_schemes = 4;
    static constexpr std::size_t small_fn_size_embedded = 8;
    using uri_parser = minimal_uri_tag;
};
```

## Error Handling

All fallible operations return `expected<void, error_code>`:

```cpp
auto result = pipe.connect(uri_view("mqtt://broker:1883"));
if (!result) {
    // result.error() == error_code::connection_failed
}
```

Error codes: `buffer_overflow`, `not_connected`, `connection_failed`, `invalid_argument`, `out_of_range`, `capacity_exceeded`, `disconnected`, `timeout`, `stage_dropped`, `invalid_uri`.

## Building

### Requirements

- C++20 compiler (GCC 14+, Clang 17+, MSVC 19.34+)
- CMake 3.20+
- GTest (for tests)

### Build & Test

```bash
mkdir build && cd build
cmake .. -DCMAKE_CXX_FLAGS="-std=c++20 -fno-rtti -fno-exceptions" -DPIPEPP_CORE_BUILD_TESTS=ON
cmake --build . --parallel
ctest --output-on-failure
```

### Docker

```bash
# Ubuntu 24.04 (GCC 14)
docker build -t pipepp-core-test . && docker run --rm pipepp-core-test

# Alpine 3.21 (GCC 14, -Os)
docker build -t pipepp-core-alpine -f Dockerfile.alpine . && docker run --rm pipepp-core-alpine

# Fedora 42 (GCC 15)
docker build -t pipepp-core-fedora -f Dockerfile.fedora . && docker run --rm pipepp-core-fedora

# Coverage report
docker build -t pipepp-core-coverage -f Dockerfile.coverage . && docker run --rm pipepp-core-coverage
```

## Test Results

| Platform | Compiler | Flags | Tests | Coverage |
|----------|----------|-------|-------|----------|
| Ubuntu 24.04 | GCC 14.2 | `-fno-rtti -fno-exceptions` | 184/184 | 99% |
| Alpine 3.21 | GCC 14.2 | `-fno-rtti -fno-exceptions -Os` | 184/184 | — |
| Fedora 42 | GCC 15.2 | `-fno-rtti -fno-exceptions` | 184/184 | — |

## Directory Layout

```
pipepp-core/
├── include/pipepp/
│   └── pipepp.hpp              # Umbrella header
├── include/pipepp/core/
│   ├── adapt.hpp               # range_source, adapt()
│   ├── any_source.hpp          # Type-erased source (manual vtable)
│   ├── as_range.hpp            # sink_buffer for range interop
│   ├── concepts.hpp            # BusSource concept
│   ├── config.hpp              # default_config, embedded_config, URI tags
│   ├── connect_to.hpp          # Cross-pipeline connection, with_remap()
│   ├── error_code.hpp          # error_code enum
│   ├── expected.hpp            # expected<T, E> backfill
│   ├── fixed_string.hpp        # Compile-time fixed-size string
│   ├── message.hpp             # message_view, basic_message<Config>
│   ├── message_callback.hpp    # message_callback<Config> alias
│   ├── pipeline.hpp             # basic_pipeline<Source, Config>
│   ├── pipeline_ops.hpp        # transform, filter, subscribe, configure, sink, execute
│   ├── small_function.hpp      # Fixed-capacity move-only function wrapper
│   ├── source_registry.hpp     # Source factory registry
│   ├── spinlock.hpp            # Spinlock + spinlock_guard
│   ├── stages.hpp              # config_fn, process_fn, sink_fn
│   └── uri.hpp                 # uri_view, minimal/rfc3986 parsers, basic_uri<Config>
├── test/
│   ├── unit/                   # Unit tests
│   ├── integration/            # Integration tests
│   └── system/                 # System/end-to-end tests
├── Dockerfile                  # Ubuntu 24.04 build
├── Dockerfile.alpine           # Alpine 3.21 build
├── Dockerfile.fedora           # Fedora 42 build
└── Dockerfile.coverage         # Coverage report (gcovr)
```

## License

MIT
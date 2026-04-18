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

// 1. Define a source (must satisfy BusSource concept)
struct MySource {
    result connect(uri_view = {}) { connected_ = true; return {}; }
    result disconnect() { connected_ = false; return {}; }
    bool is_connected() const { return connected_; }
    result subscribe(std::string_view topic, int qos) { return {}; }
    result publish(std::string_view, std::span<const std::byte>, int) { return {}; }
    void set_message_callback(message_callback<default_config> cb) { callback_ = std::move(cb); }
    void poll() {}

    bool connected_ = false;
    message_callback<default_config> callback_;
};

// 2. Create a pipeline and compose with pipe syntax
auto pipe = basic_pipeline<MySource>(MySource{})
    | subscribe("sensors/#", 0)
    | process([](basic_message<default_config>& msg) { return true; })
    | filter([](basic_message<default_config>& msg) { return true; })
    | sink([](const message_view& mv) { /* handle result */ });

// 3. Emit a message through the pipeline
std::byte data[] = {std::byte{0x01}};
message_view mv("sensors/temp", data, 0);
pipe.emit(mv);
```

## Modules

| Module | Description | Status |
|--------|-------------|--------|
| `pipepp-core` | Generic pipeline framework (header-only) | Complete |
| `pipepp-mqtt` | MQTT specialization (depends on Eclipse Paho C++) | Planned |

## Architecture

### BusSource Concept

Any type implementing these seven methods satisfies `BusSource<T, Config>`:

```cpp
struct MySource {
    result connect(uri_view uri);                              // connect to broker
    result disconnect();                                       // disconnect
    bool is_connected() const;                                 // connection state
    result subscribe(std::string_view topic, int qos);         // subscribe to topic
    result publish(std::string_view topic,                     // publish message
                   std::span<const std::byte> payload, int qos);
    void set_message_callback(message_callback<Config> cb);    // set inbound handler
    void poll();                                               // drive I/O
};
```

### Pipeline Elements

Every pipeline operation falls into one of three categories:

- **Configuration** — modifies the source (`configure(fn)`)
- **Processing** — transforms or filters messages (`process()`, `filter()`)
- **Terminal** — consumes output (`sink()`)

### Method and Pipe Syntax

Every `.method()` has a `| stage` equivalent:

```cpp
basic_pipeline<MySource> pipe(MySource{});

// Method syntax
pipe.add_stage(fn).set_sink(consumer);

// Pipe syntax (identical behavior)
auto p = basic_pipeline<MySource>(MySource{})
    | process(fn)
    | filter(pred)
    | sink(consumer)
    | configure([](auto& src) -> result { return {}; })
    | subscribe("topic/#", 1);

// Free function equivalence
execute(pipe);   // same as pipe.run()
```

### Running and Stopping

```cpp
auto pipe = basic_pipeline<MySource>(MySource{})
    | subscribe("topic/#", 0)
    | sink([](const message_view&) {});

// Blocking event loop — calls source_.poll() until stop()
auto result = pipe.run();
EXPECT_TRUE(result.has_value());

// stop() signals the event loop to exit
pipe.stop();

// Check if running
pipe.is_running();

// Single poll tick (for external event loop integration)
if (pipe.poll_once()) { /* polled successfully */ }
```

### Emitting Messages

```cpp
std::byte payload[] = {std::byte{0x42}};
message_view mv("topic/path", payload, 1);

pipe.emit(mv);   // synchronous: lock → process stages → sink → unlock
```

### Capacity Error Detection

```cpp
auto pipe = basic_pipeline<MySource>(MySource{});
for (int i = 0; i < 20; ++i)
    pipe.add_stage(fn);  // exceeds max_stages → sets pending error

auto check = pipe.check();
if (!check) {
    // check.error() == error_code::capacity_exceeded
}

auto run_result = pipe.run();  // also returns the error
```

### Cross-Pipeline Connection

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

### Range Interop

```cpp
// Adapt a range of message_view into a source
message_view msgs[] = { mv1, mv2, mv3 };
auto src = adapt(msgs);   // returns range_source<const message_view*>

auto pipe = basic_pipeline<decltype(src)>(std::move(src))
    | process([](basic_message<default_config>&) { return true; })
    | sink([](const message_view&) {});

// Collect output into a fixed-size buffer
sink_buffer<default_config> buf;
auto pipe2 = basic_pipeline<MySource>(MySource{})
    | sink([&](const message_view& mv) { buf.push(mv); });
```

### Type-Erased Source

```cpp
any_source<default_config> src = any_source<default_config>(MySource{});
src.connect(uri);
src.subscribe("topic", 0);
src.poll();
```

### Source Registry

```cpp
source_registry<default_config> registry;
registry.register_scheme<MySource>("mqtt");

auto src = registry.create("mqtt");                        // plain scheme
auto src2 = registry.create("mqtt://broker:1883/topic");   // full URI
```

### Placement-New Factory

For environments without heap allocation:

```cpp
alignas(pipeline_align<MySource>())
    std::byte buffer[pipeline_size<MySource>()];

auto* pipe = pipeline_allocator<MySource>::create(buffer, sizeof(buffer), MySource{});
pipe->run();
pipeline_allocator<MySource>::destroy(pipe);
```

### URI Support

Compile-time selectable URI parser via Config:

```cpp
struct my_config : default_config {
    using uri_parser = minimal_uri_tag;   // scheme://host:port/path?query#fragment
};

struct my_config_rfc : default_config {
    using uri_parser = rfc3986_uri_tag;   // full RFC 3986 (IPv6, userinfo)
};
```

Owned URI storage prevents dangling:

```cpp
auto uri = basic_uri<default_config>::parse("mqtt://broker:1883/topic");
uri.view().scheme;   // "mqtt"
uri.view().host;     // "broker"
uri.truncated();     // false (or true if input exceeded max_uri_len)
```

### Thread Safety

Thread safety uses a **spinlock-only** approach:
- `spinlock_guard` around `emit()` and `process_message()`
- `std::atomic<bool>` for the `running_` flag
- `emit()` is synchronous — blocks until all stages + sink complete
- No SPSC queue, no lock-free queue, no `flush()`

## Configuration

Customize buffer sizes and behavior via a Config struct:

```cpp
// Provided configs (in config.hpp)
struct default_config {
    using uri_parser = minimal_uri_tag;
    static constexpr std::size_t max_uri_len         = 256;
    static constexpr std::size_t max_topic_len        = 256;
    static constexpr std::size_t max_payload_len      = 4096;
    static constexpr std::size_t max_stages           = 8;
    static constexpr std::size_t max_config_ops       = 4;
    static constexpr std::size_t max_subscriptions    = 8;
    static constexpr std::size_t small_fn_size        = 128;
    static constexpr std::size_t source_size          = 2048;
    static constexpr std::size_t max_scheme_len       = 16;
    static constexpr std::size_t max_schemes          = 4;
};

struct embedded_config {
    using uri_parser = minimal_uri_tag;
    static constexpr std::size_t max_uri_len         = 128;
    static constexpr std::size_t max_topic_len        = 64;
    static constexpr std::size_t max_payload_len      = 512;
    static constexpr std::size_t max_stages           = 4;
    static constexpr std::size_t max_config_ops       = 2;
    static constexpr std::size_t max_subscriptions    = 4;
    static constexpr std::size_t small_fn_size        = 64;
    static constexpr std::size_t source_size          = 512;
    static constexpr std::size_t max_scheme_len       = 8;
    static constexpr std::size_t max_schemes          = 2;
};

// Custom config example
struct my_config {
    using uri_parser = rfc3986_uri_tag;
    static constexpr std::size_t max_uri_len         = 512;
    static constexpr std::size_t max_topic_len        = 128;
    static constexpr std::size_t max_payload_len      = 1024;
    static constexpr std::size_t max_stages           = 6;
    static constexpr std::size_t max_config_ops       = 3;
    static constexpr std::size_t max_subscriptions    = 6;
    static constexpr std::size_t small_fn_size        = 96;
    static constexpr std::size_t source_size          = 1024;
    static constexpr std::size_t max_scheme_len       = 16;
    static constexpr std::size_t max_schemes          = 4;
};
```

## Error Handling

All fallible operations return `expected<void, error_code>`:

```cpp
auto result = pipe.run();
if (!result) {
    switch (result.error()) {
    case error_code::none:               break;
    case error_code::capacity_exceeded:  break;
    case error_code::connection_failed:  break;
    // ...
    }
}
```

Error codes: `none`, `buffer_overflow`, `not_connected`, `connection_failed`, `invalid_argument`, `out_of_range`, `capacity_exceeded`, `disconnected`, `timeout`, `stage_dropped`, `invalid_uri`.

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
docker build -t pipepp-core-ubuntu . && docker run --rm pipepp-core-ubuntu

# Alpine 3.21 (GCC 14, -Os)
docker build -t pipepp-core-alpine -f Dockerfile.alpine . && docker run --rm pipepp-core-alpine

# Fedora 42 (GCC 15)
docker build -t pipepp-core-fedora -f Dockerfile.fedora . && docker run --rm pipepp-core-fedora

# Coverage report
docker build -t pipepp-core-coverage -f Dockerfile.coverage . && docker run --rm pipepp-core-coverage
```

## Test Results

| Platform | Compiler | Flags | Tests |
|----------|----------|-------|-------|
| Ubuntu 24.04 | GCC 14.2 | `-fno-rtti -fno-exceptions` | 223/223 |
| Alpine 3.21 | GCC 14.2 | `-fno-rtti -fno-exceptions -Os` | 223/223 |
| Fedora 42 | GCC 15.2 | `-fno-rtti -fno-exceptions` | 223/223 |

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
│   ├── pipeline.hpp            # basic_pipeline<Source, Config>
│   ├── pipeline_factory.hpp    # pipeline_allocator for placement-new
│   ├── pipeline_ops.hpp        # process, filter, subscribe, configure, sink, execute
│   ├── small_function.hpp      # Fixed-capacity move-only function wrapper
│   ├── source_registry.hpp     # Source factory registry
│   ├── spinlock.hpp            # Spinlock + spinlock_guard
│   ├── stages.hpp              # config_fn, process_fn, sink_fn
│   └── uri.hpp                 # uri_view, minimal/rfc3986 parsers, basic_uri<Config>
├── test/
│   ├── unit/                   # Unit tests (17 files)
│   ├── integration/            # Integration tests (2 files)
│   └── system/                 # System/end-to-end tests (2 files)
├── Dockerfile                  # Ubuntu 24.04 build
├── Dockerfile.alpine           # Alpine 3.21 build
├── Dockerfile.fedora           # Fedora 42 build
└── Dockerfile.coverage         # Coverage report (gcovr)
```

## License

MIT

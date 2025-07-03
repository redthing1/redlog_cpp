
# redlog

a modern c++ header-only logging library

## features

- beautiful aligned output with automatic color detection
- structured logging with key-value fields
- immutable scoped loggers for hierarchical context
- thread-safe by design
- zero dependencies, single header
- c++17 compatible

## quick start

```cpp
#include <redlog.hpp>

int main() {
    auto log = redlog::get_logger("app");
    
    log.info("hello world");
    log.info("user login", 
             redlog::field("username", "alice"),
             redlog::field("success", true));
    
    auto db_log = log.with_name("database");
    db_log.error("connection failed", redlog::field("host", "localhost"));
}
```

## build examples

```bash
cmake -B build
cmake --build build --parallel
```

## usage

### basic logging

```cpp
auto log = redlog::get_logger("myapp");

log.critical("system failure");
log.error("something went wrong");
log.warn("potential issue");
log.info("general information");
log.debug("debugging details");
```

### short forms

```cpp
log.crt("critical");  // same as critical()
log.err("error");     // same as error()
log.inf("info");      // same as info()
log.dbg("debug");     // same as debug()
```

### structured fields

```cpp
log.info("user action",
         redlog::field("user_id", 12345),
         redlog::field("action", "login"),
         redlog::field("ip", "192.168.1.100"));
```

### scoped loggers

```cpp
auto request_log = log.with_field("request_id", 12345)
                      .with_field("method", "POST");

auto handler_log = request_log.with_name("handler");
handler_log.info("processing request");
```

### printf-style formatting

```cpp
log.info_f("user %s logged in from %s", username, ip_address);
log.error_f("failed to connect to %s:%s", host, port);
```

### configuration

```cpp
// set minimum log level
redlog::set_level(redlog::level::debug);

// use plain theme (no colors)
redlog::set_theme(redlog::themes::plain);
```

## integration

### cmake project

add redlog as a subdirectory to your cmake project:

```cmake
add_subdirectory(redlog)
target_link_libraries(your_target PRIVATE redlog::redlog)
```

### header-only

copy `include/redlog.hpp` to your project and include it directly:

```cpp
#include "redlog.hpp"
// ready to use - no linking required
```

### consume

redlog is header-only with zero dependencies. simply:

1. copy the header file or add as git submodule
2. include in your source files  
3. compile with c++17 or later

## testing

```bash
cmake -B build -DREDLOG_BUILD_TESTS=ON
cmake --build build --parallel
./build/tests/redlog_tests
```

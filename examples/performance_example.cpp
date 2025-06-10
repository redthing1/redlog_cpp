#include <chrono>
#include <random>
#include <redlog/redlog.hpp>
#include <vector>

// Benchmark utility
class benchmark {
  std::chrono::steady_clock::time_point start_;

public:
  benchmark() : start_(std::chrono::steady_clock::now()) {}

  long long elapsed_ns() const {
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_).count();
  }

  long long elapsed_us() const { return elapsed_ns() / 1000; }

  long long elapsed_ms() const { return elapsed_us() / 1000; }
};

int main() {
  using namespace redlog;

  auto log = get_logger("perf");
  log.info("Performance testing started");

  const int iterations = 1000000;

  // Test 1: Disabled level overhead
  {
    set_level(level::warn); // Disable info and debug
    benchmark bench;

    for (int i = 0; i < iterations; ++i) {
      log.debug("Disabled message");
    }

    log.info(
        "Disabled level test completed", field("iterations", iterations), field("total_time_us", bench.elapsed_us()),
        field("avg_time_ns", bench.elapsed_ns() / iterations)
    );
  }

  // Test 2: Simple message logging
  {
    set_level(level::info);
    benchmark bench;

    for (int i = 0; i < iterations / 100; ++i) { // Fewer iterations since these actually log
      log.info("Simple message");
    }

    log.info(
        "Simple message test completed", field("iterations", iterations / 100),
        field("total_time_us", bench.elapsed_us()), field("avg_time_ns", bench.elapsed_ns() / (iterations / 100))
    );
  }

  // Test 3: Message with fields
  {
    benchmark bench;

    for (int i = 0; i < iterations / 100; ++i) {
      log.info("Message with fields", field("id", i), field("name", "test"), field("value", 3.14));
    }

    log.info(
        "Fields test completed", field("iterations", iterations / 100), field("total_time_us", bench.elapsed_us()),
        field("avg_time_ns", bench.elapsed_ns() / (iterations / 100))
    );
  }

  // Test 4: Logger creation and scoping
  {
    benchmark bench;

    for (int i = 0; i < iterations / 100; ++i) {
      auto scoped_log = log.with_name("scoped").with_field("iteration", i);
      scoped_log.info("Scoped message");
    }

    log.info(
        "Logger scoping test completed", field("iterations", iterations / 100),
        field("total_time_us", bench.elapsed_us()), field("avg_time_ns", bench.elapsed_ns() / (iterations / 100))
    );
  }

  // Test 5: Printf vs structured logging comparison
  {
    const int printf_iterations = iterations / 1000;

    // Printf-style
    benchmark printf_bench;
    for (int i = 0; i < printf_iterations; ++i) {
      log.info_f("Printf style: %s %s %s", i, "test", 3.14);
    }
    auto printf_time = printf_bench.elapsed_us();

    // Structured logging
    benchmark struct_bench;
    for (int i = 0; i < printf_iterations; ++i) {
      log.info("Structured style", field("id", i), field("name", "test"), field("value", 3.14));
    }
    auto struct_time = struct_bench.elapsed_us();

    log.info(
        "Printf vs structured comparison", field("iterations", printf_iterations), field("printf_time_us", printf_time),
        field("struct_time_us", struct_time), field("printf_avg_ns", printf_time * 1000 / printf_iterations),
        field("struct_avg_ns", struct_time * 1000 / printf_iterations)
    );
  }

  // Test 6: Memory allocation patterns
  {
    // Test with many short-lived loggers
    benchmark bench;

    for (int i = 0; i < iterations / 1000; ++i) {
      auto temp_log = get_logger("temp")
                          .with_field("session", i)
                          .with_field("user", "testuser")
                          .with_name("module")
                          .with_field("operation", "test");
      temp_log.info("Temporary logger message");
    }

    log.info(
        "Memory allocation test completed", field("iterations", iterations / 1000),
        field("total_time_us", bench.elapsed_us()), field("avg_time_ns", bench.elapsed_ns() / (iterations / 1000))
    );
  }

  log.info("Performance testing completed");

  return 0;
}
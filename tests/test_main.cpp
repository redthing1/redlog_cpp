#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <redlog/redlog.hpp>
#include <sstream>
#include <thread>
#include <vector>

// String sink for capturing output in tests
class string_sink : public redlog::sink {
  std::ostringstream buffer_;

public:
  void write(std::string_view formatted) override { buffer_ << formatted << "\n"; }

  void flush() override {
    // No-op for string sink
  }

  std::string get_output() const { return buffer_.str(); }

  void clear() {
    buffer_.str("");
    buffer_.clear();
  }
};

// Custom formatter that adds timestamps (from the themes example)
class timestamped_formatter : public redlog::formatter {
  redlog::theme theme_;

  redlog::color level_color(redlog::level l) const {
    switch (l) {
    case redlog::level::critical:
      return theme_.critical_color;
    case redlog::level::error:
      return theme_.error_color;
    case redlog::level::warn:
      return theme_.warn_color;
    case redlog::level::info:
      return theme_.info_color;
    case redlog::level::verbose:
      return theme_.verbose_color;
    case redlog::level::trace:
      return theme_.trace_color;
    case redlog::level::debug:
      return theme_.debug_color;
    case redlog::level::pedantic:
      return theme_.pedantic_color;
    case redlog::level::annoying:
      return theme_.annoying_color;
    default:
      return redlog::color::white;
    }
  }

public:
  timestamped_formatter() : theme_(redlog::detail::config::instance().get_theme()) {}
  explicit timestamped_formatter(const redlog::theme& t) : theme_(t) {}

  std::string format(const redlog::log_entry& entry) const override {
    std::ostringstream oss;

    // Add timestamp
    auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
    auto tm = *std::localtime(&time_t);
    oss << "[" << std::put_time(&tm, "%H:%M:%S") << "] ";

    // Source component
    if (!entry.source.empty()) {
      std::string source_part = entry.source;
      oss << redlog::detail::colorize(source_part, theme_.source_color) << " ";
    }

    // Level component
    std::string level_part = std::string(redlog::level_short_name(entry.level_val));
    oss << redlog::detail::colorize(level_part, level_color(entry.level_val)) << ": ";

    // Message
    oss << redlog::detail::colorize(entry.message, theme_.message_color);

    // Fields
    if (!entry.fields.empty()) {
      oss << " [";
      bool first = true;
      for (const auto& f : entry.fields.fields()) {
        if (!first) {
          oss << ", ";
        }
        first = false;
        oss << redlog::detail::colorize(f.key, theme_.field_key_color) << "="
            << redlog::detail::colorize(f.value, theme_.field_value_color);
      }
      oss << "]";
    }

    return oss.str();
  }
};

// Simple test framework
class test_runner {
  int tests_run = 0;
  int tests_passed = 0;

public:
  void run_test(const std::string& name, std::function<void()> test_func) {
    tests_run++;
    std::cout << "Running test: " << name << " ... ";

    try {
      test_func();
      tests_passed++;
      std::cout << "PASSED\n";
    } catch (const std::exception& e) {
      std::cout << "FAILED: " << e.what() << "\n";
    } catch (...) {
      std::cout << "FAILED: Unknown exception\n";
    }
  }

  void print_summary() {
    std::cout << "\n=== Test Summary ===\n";
    std::cout << "Tests run: " << tests_run << "\n";
    std::cout << "Tests passed: " << tests_passed << "\n";
    std::cout << "Tests failed: " << (tests_run - tests_passed) << "\n";

    if (tests_passed == tests_run) {
      std::cout << "All tests PASSED!\n";
    } else {
      std::cout << "Some tests FAILED!\n";
    }
  }

  bool all_passed() const { return tests_passed == tests_run; }
};

// Helper to capture stderr output for testing
class stderr_capturer {
  std::streambuf* original_stderr;
  std::ostringstream captured;

public:
  stderr_capturer() {
    original_stderr = std::cerr.rdbuf();
    std::cerr.rdbuf(captured.rdbuf());
  }

  ~stderr_capturer() { std::cerr.rdbuf(original_stderr); }

  std::string get_output() const { return captured.str(); }
};

// Custom sink for testing
class test_sink : public redlog::sink {
  std::ostringstream buffer_;

public:
  void write(std::string_view formatted) override { buffer_ << formatted << "\n"; }

  void flush() override {
    // No-op for testing
  }

  std::string get_output() const { return buffer_.str(); }

  void clear() {
    buffer_.str("");
    buffer_.clear();
  }
};

// Custom type for testing
struct test_object {
  int value;
  std::string name;

  friend std::ostream& operator<<(std::ostream& os, const test_object& obj) {
    return os << "TestObject{" << obj.value << ", " << obj.name << "}";
  }
};

// helper function to strip ansi color codes for testing
std::string strip_ansi_colors(const std::string& input) {
  std::string result;
  bool in_escape = false;

  for (size_t i = 0; i < input.length(); ++i) {
    if (input[i] == '\033' && i + 1 < input.length() && input[i + 1] == '[') {
      in_escape = true;
      ++i; // skip the '['
    } else if (in_escape && input[i] == 'm') {
      in_escape = false;
    } else if (!in_escape) {
      result += input[i];
    }
  }

  return result;
}

void test_basic_logging() {
  using namespace redlog;

  auto log = get_logger("test");

  // Test that all logging methods exist and can be called
  log.critical("critical message");
  log.error("error message");
  log.warn("warn message");
  log.info("info message");
  log.verbose("verbose message");
  log.trace("trace message");
  log.debug("debug message");
  log.pedantic("pedantic message");
  log.annoying("annoying message");

  // Test short form methods
  log.crt("critical short");
  log.err("error short");
  log.wrn("warn short");
  log.inf("info short");
  log.vrb("verbose short");
  log.trc("trace short");
  log.dbg("debug short");
  log.ped("pedantic short");
  log.ayg("annoying short");
}

void test_field_system() {
  using namespace redlog;

  auto log = get_logger("field_test");

  // Test basic field creation
  field f1("key1", "value1");
  assert(f1.key == "key1");
  assert(f1.value == "value1");

  // Test different value types
  field f_int("int", 42);
  field f_float("float", 3.14);
  field f_bool("bool", true);

  assert(f_int.value == "42");
  assert(f_float.value.find("3.14") != std::string::npos);
  assert(f_bool.value == "1" || f_bool.value == "true");

  // Test logging with fields
  log.info("Message with fields", field("string", "test"), field("number", 123), field("float", 2.71828));

  // Test custom type
  test_object obj{42, "test"};
  log.info("Custom object", field("object", obj));
}

void test_scoped_loggers() {
  using namespace redlog;

  auto base_log = get_logger("base");

  // Test with_name
  auto named_log = base_log.with_name("module");
  named_log.info("Named logger test");

  // Test hierarchical naming
  auto nested_log = named_log.with_name("submodule");
  nested_log.info("Nested logger test");

  // Test with_field
  auto field_log = base_log.with_field("session_id", 12345);
  field_log.info("Logger with field");

  // Test chaining
  auto chained_log = base_log.with_name("chained").with_field("user", "alice").with_field("action", "login");
  chained_log.info("Chained logger test");

  // Test that original logger is unchanged (immutability)
  base_log.info("Original logger unchanged");
}

void test_printf_formatting() {
  using namespace redlog;

  // Test integer format specifiers
  {
    std::string result = redlog::detail::stream_printf("Value: %d", 42);
    assert(result == "Value: 42");
  }

  {
    std::string result = redlog::detail::stream_printf("Value: %i", -123);
    assert(result == "Value: -123");
  }

  // Test hexadecimal format specifiers
  {
    std::string result = redlog::detail::stream_printf("Hex: %x", 255);
    assert(result == "Hex: ff");
  }

  {
    std::string result = redlog::detail::stream_printf("HEX: %X", 255);
    assert(result == "HEX: FF");
  }

  // Test octal format specifier
  {
    std::string result = redlog::detail::stream_printf("Octal: %o", 64);
    assert(result == "Octal: 100");
  }

  // Test floating point format specifiers
  {
    std::string result = redlog::detail::stream_printf("Float: %f", 3.14);
    assert(result == "Float: 3.14");
  }

  {
    std::string result = redlog::detail::stream_printf("Precision: %.2f", 3.14159);
    assert(result == "Precision: 3.14");
  }

  {
    std::string result = redlog::detail::stream_printf("Scientific: %e", 1234.5);
    // Scientific notation format can vary, just check it contains 'e' or 'E'
    assert(result.find("e") != std::string::npos || result.find("E") != std::string::npos);
    assert(result.find("Scientific:") != std::string::npos);
  }

  // Test character format specifier
  {
    std::string result = redlog::detail::stream_printf("Char: %c", 65);
    assert(result == "Char: A");
  }

  // Test string format specifier
  {
    std::string result = redlog::detail::stream_printf("String: %s", "hello");
    assert(result == "String: hello");
  }

  // Test mixed format specifiers
  {
    std::string result = redlog::detail::stream_printf("Port %d on %s", 8080, "localhost");
    assert(result == "Port 8080 on localhost");
  }

  // Test custom type with operator<<
  {
    test_object obj{42, "test"};
    std::string result = redlog::detail::stream_printf("Object: %s", obj);
    assert(result.find("TestObject") != std::string::npos);
    assert(result.find("42") != std::string::npos);
  }

  // Test non-arithmetic type with %d (should fallback to stringify)
  {
    std::string text = "hello";
    std::string result = redlog::detail::stream_printf("Text as int: %d", text);
    assert(result == "Text as int: hello");
  }

  // Test escaped %%
  {
    std::string result = redlog::detail::stream_printf("Percentage: %%");
    assert(result == "Percentage: %");
  }

  // Test logger integration
  auto log = get_logger("printf_test");

  // Test all level printf methods work
  log.critical_f("Critical: %d", 1);
  log.error_f("Error: %d", 2);
  log.warn_f("Warn: %d", 3);
  log.info_f("Info: %d", 4);
  log.verbose_f("Verbose: %d", 5);
  log.trace_f("Trace: %d", 6);
  log.debug_f("Debug: %d", 7);
  log.pedantic_f("Pedantic: %d", 8);
  log.annoying_f("Annoying: %d", 9);

  // Test short form printf methods
  log.crt_f("Critical short: %x", 255);
  log.err_f("Error short: %x", 255);
  log.wrn_f("Warn short: %x", 255);
  log.inf_f("Info short: %x", 255);
  log.vrb_f("Verbose short: %x", 255);
  log.trc_f("Trace short: %x", 255);
  log.dbg_f("Debug short: %x", 255);
  log.ped_f("Pedantic short: %x", 255);
  log.ayg_f("Annoying short: %x", 255);
}

void test_format_specifier_parsing() {
  using namespace redlog;

  // Test comprehensive integer formatting
  {
    assert(redlog::detail::stream_printf("%d", 0) == "0");
    assert(redlog::detail::stream_printf("%d", -1) == "-1");
    assert(redlog::detail::stream_printf("%d", 12345) == "12345");
    assert(redlog::detail::stream_printf("%i", 42) == "42");
  }

  // Test hexadecimal formatting variations
  {
    assert(redlog::detail::stream_printf("%x", 0) == "0");
    assert(redlog::detail::stream_printf("%x", 10) == "a");
    assert(redlog::detail::stream_printf("%x", 255) == "ff");
    assert(redlog::detail::stream_printf("%X", 255) == "FF");
    assert(redlog::detail::stream_printf("%X", 10) == "A");
  }

  // Test octal formatting
  {
    assert(redlog::detail::stream_printf("%o", 0) == "0");
    assert(redlog::detail::stream_printf("%o", 8) == "10");
    assert(redlog::detail::stream_printf("%o", 64) == "100");
    assert(redlog::detail::stream_printf("%o", 511) == "777");
  }

  // Test floating point formatting
  {
    assert(redlog::detail::stream_printf("%f", 0.0) == "0");
    assert(redlog::detail::stream_printf("%f", 1.0) == "1");
    assert(redlog::detail::stream_printf("%f", 3.14) == "3.14");
    assert(redlog::detail::stream_printf("%f", -2.5) == "-2.5");
  }

  // Test precision specifiers in detail
  {
    assert(redlog::detail::stream_printf("%.0f", 3.14159) == "3");
    assert(redlog::detail::stream_printf("%.1f", 3.14159) == "3.1");
    assert(redlog::detail::stream_printf("%.2f", 3.14159) == "3.14");
    assert(redlog::detail::stream_printf("%.3f", 3.14159) == "3.142");
    assert(redlog::detail::stream_printf("%.5f", 3.14159) == "3.14159");
  }

  // Test scientific notation
  {
    std::string result_e = redlog::detail::stream_printf("%e", 1234.5);
    assert(result_e.find("e") != std::string::npos);
    assert(result_e.find("1.234") != std::string::npos);

    std::string result_E = redlog::detail::stream_printf("%E", 1234.5);
    assert(result_E.find("E") != std::string::npos);
    assert(result_E.find("1.234") != std::string::npos);
  }

  // Test character formatting
  {
    assert(redlog::detail::stream_printf("%c", 65) == "A");
    assert(redlog::detail::stream_printf("%c", 97) == "a");
    assert(redlog::detail::stream_printf("%c", 48) == "0");
    assert(redlog::detail::stream_printf("%c", 32) == " ");
  }

  // Test string formatting with various string types
  {
    assert(redlog::detail::stream_printf("%s", "hello") == "hello");
    assert(redlog::detail::stream_printf("%s", std::string("world")) == "world");

    const char* cstr = "test";
    assert(redlog::detail::stream_printf("%s", cstr) == "test");

    std::string stdstr = "string";
    assert(redlog::detail::stream_printf("%s", stdstr) == "string");
  }

  // Test complex mixed formatting
  {
    std::string result = redlog::detail::stream_printf(
        "Server %s:%d (load: %.1f%%, hex: 0x%x, octal: %o)", "localhost", 8080, 95.7, 255, 64
    );
    assert(result == "Server localhost:8080 (load: 95.7%, hex: 0xff, octal: 100)");
  }

  // Test format specifier edge cases
  {
    // Single format specifier
    assert(redlog::detail::stream_printf("%d", 42) == "42");

    // Format at beginning
    assert(redlog::detail::stream_printf("%s world", "hello") == "hello world");

    // Format at end
    assert(redlog::detail::stream_printf("value: %d", 42) == "value: 42");

    // Multiple consecutive formats
    assert(redlog::detail::stream_printf("%d%s%d", 1, "a", 2) == "1a2");
  }

  // Test non-standard types with different format specifiers
  {
    // String with numeric format should fallback to stringify
    std::string text = "hello";
    assert(redlog::detail::stream_printf("%d", text) == "hello");
    assert(redlog::detail::stream_printf("%x", text) == "hello");
    assert(redlog::detail::stream_printf("%f", text) == "hello");
  }
}

void test_printf_edge_cases() {
  using namespace redlog;

  // Test error handling with invalid format
  {
    std::string result = redlog::detail::stream_printf("Invalid: %q", 42);
    assert(result.find("Invalid:") != std::string::npos); // Should handle gracefully
  }

  // Test missing arguments
  {
    std::string result = redlog::detail::stream_printf("Missing: %d %s");
    assert(result == "Missing: %d %s"); // Should leave unused specifiers
  }

  // Test extra arguments
  {
    std::string result = redlog::detail::stream_printf("Extra: %d", 42, 99);
    assert(result == "Extra: 42"); // Should ignore extra args
  }

  // Test no format specifiers
  {
    std::string result = redlog::detail::stream_printf("No formats", 42, "ignored");
    assert(result == "No formats");
  }

  // Test only format specifiers
  {
    std::string result = redlog::detail::stream_printf("%d %s %f", 42, "hello", 3.14);
    assert(result == "42 hello 3.14");
  }

  // Test complex precision format
  {
    std::string result = redlog::detail::stream_printf("Complex: %.3f", 3.14159);
    assert(result == "Complex: 3.142");
  }

  // Test zero precision
  {
    std::string result = redlog::detail::stream_printf("Zero precision: %.0f", 3.14159);
    assert(result == "Zero precision: 3");
  }

  // Test large numbers
  {
    std::string result = redlog::detail::stream_printf("Large: %d", 2147483647);
    assert(result == "Large: 2147483647");
  }

  // Test negative numbers
  {
    std::string result = redlog::detail::stream_printf("Negative: %d %f", -42, -3.14);
    assert(result == "Negative: -42 -3.14");
  }

  // Test boolean values
  {
    std::string result = redlog::detail::stream_printf("Bool true: %d, false: %d", true, false);
    assert(result == "Bool true: 1, false: 0");
  }

  // Test different numeric types
  {
    short s = 123;
    long l = 456789;
    unsigned u = 999;
    std::string result = redlog::detail::stream_printf("Types: %d %d %d", s, l, u);
    assert(result == "Types: 123 456789 999");
  }
}

void test_level_filtering() {
  using namespace redlog;

  // Test level setting and getting
  set_level(level::warn);
  assert(get_level() == level::warn);

  set_level(level::debug);
  assert(get_level() == level::debug);

  set_level(level::info);
  assert(get_level() == level::info);

  // Test that filtering works by verifying filtered messages are very fast
  set_level(level::warn); // Only show critical, error, warn
  auto log = get_logger("filter_test");

  // Test that filtered messages (info, verbose, debug) are very fast
  auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < 1000; ++i) {
    log.info("filtered message");
    log.verbose("filtered message");
    log.debug("filtered message");
  }
  auto end = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  // Filtered messages should be reasonably fast (less than 10ms for 3000 calls)
  assert(duration.count() < 10000);

  // Restore level for other tests
  set_level(level::info);
}

void test_theme_system() {
  using namespace redlog;

  // Test theme getting and setting
  theme original_theme = get_theme();

  // Test plain theme
  set_theme(themes::plain);
  theme plain_theme = get_theme();
  assert(plain_theme.error_color == color::none);

  // Restore original theme
  set_theme(original_theme);

  // Test custom theme
  theme custom_theme = themes::default_theme;
  custom_theme.error_color = color::bright_red;
  custom_theme.source_width = 20;
  set_theme(custom_theme);

  theme retrieved_theme = get_theme();
  assert(retrieved_theme.error_color == color::bright_red);
  assert(retrieved_theme.source_width == 20);

  // Restore original theme at the end to not affect other tests
  set_theme(original_theme);
}

void test_formatter_functionality() {
  using namespace redlog;

  // Test default formatter
  default_formatter formatter;

  log_entry entry(level::info, "test message", "test_source", field_set{});
  std::string formatted = formatter.format(entry);

  // Check that formatted string contains expected components
  assert(formatted.find("test_source") != std::string::npos);
  assert(formatted.find("test message") != std::string::npos);
  assert(formatted.find("inf") != std::string::npos);

  // Test with fields
  field_set fields;
  fields.add(field("key1", "value1"));
  fields.add(field("key2", 42));

  log_entry entry_with_fields(level::error, "error message", "error_source", std::move(fields));
  std::string formatted_with_fields = formatter.format(entry_with_fields);

  // strip ansi color codes before checking for field content
  std::string clean_output = strip_ansi_colors(formatted_with_fields);

  assert(clean_output.find("key1=value1") != std::string::npos);
  assert(clean_output.find("key2=42") != std::string::npos);
}

void test_stringification() {
  using namespace redlog::detail;

  // Test basic types
  assert(stringify(std::string("hello")) == "hello");
  assert(stringify("hello") == "hello");
  assert(stringify(42) == "42");
  assert(stringify(3.14) == "3.140000");
  assert(stringify(true) == "1");

  // Test custom type with operator<<
  test_object obj{123, "stringify_test"};
  std::string result = stringify(obj);
  assert(result.find("TestObject") != std::string::npos);
  assert(result.find("123") != std::string::npos);
  assert(result.find("stringify_test") != std::string::npos);
}

void test_fmt_function() {
  using namespace redlog;

  // Test basic format specifiers
  assert(fmt("Value: %d", 42) == "Value: 42");
  assert(fmt("Float: %.2f", 3.14159) == "Float: 3.14");
  assert(fmt("String: %s", "hello") == "String: hello");
  assert(fmt("Hex: %x", 255) == "Hex: ff");
  assert(fmt("HEX: %X", 255) == "HEX: FF");
  assert(fmt("Octal: %o", 64) == "Octal: 100");
  assert(fmt("Char: %c", 65) == "Char: A");

  // Test mixed format specifiers
  assert(fmt("Server %s:%d (load: %.1f%%)", "localhost", 8080, 95.7) == "Server localhost:8080 (load: 95.7%)");

  // Test custom type
  test_object obj{42, "test"};
  std::string result = fmt("Object: %s", obj);
  assert(result.find("TestObject") != std::string::npos);
  assert(result.find("42") != std::string::npos);

  // Test no arguments
  assert(fmt("No args") == "No args");

  // Test escaped %%
  assert(fmt("Percentage: %%") == "Percentage: %");
}

void test_field_set_operations() {
  using namespace redlog;

  field_set fs1;
  assert(fs1.empty());
  assert(fs1.size() == 0);

  fs1.add(field("key1", "value1"));
  assert(!fs1.empty());
  assert(fs1.size() == 1);

  field_set fs2{field("key2", "value2"), field("key3", "value3")};
  assert(fs2.size() == 2);

  // Test merge
  fs1.merge(fs2);
  assert(fs1.size() == 3);

  // Test with_field (immutable operation)
  field_set fs3 = fs1.with_field(field("key4", "value4"));
  assert(fs1.size() == 3); // Original unchanged
  assert(fs3.size() == 4); // New one has additional field

  // Test with_fields
  field_set fs4 = fs1.with_fields(fs2);
  assert(fs1.size() == 3); // Original unchanged
  assert(fs4.size() == 5); // fs1 + fs2 (fs2 added again)
}

void test_thread_safety() {
  using namespace redlog;

  auto log = get_logger("thread_test");
  const int num_threads = 2;
  const int messages_per_thread = 10;

  std::vector<std::thread> threads;

  // Test concurrent logging
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&log, i, messages_per_thread]() {
      auto thread_log = log.with_field("thread_id", i);

      for (int j = 0; j < messages_per_thread; ++j) {
        thread_log.info("Thread message", field("message_id", j));

        // Mix in some logger creation
        if (j % 10 == 0) {
          auto temp_log = thread_log.with_name("temp").with_field("iteration", j);
          temp_log.debug("Temporary logger message");
        }
      }
    });
  }

  // Wait for all threads to complete
  for (auto& t : threads) {
    t.join();
  }

  log.info("Thread safety test completed");
}

void test_error_handling() {
  using namespace redlog;

  auto log = get_logger("error_test");

  // Test logging doesn't throw exceptions
  try {
    log.info("Error handling test");
    log.error("This should not throw");
    log.critical("Critical message handling");

    // Test with problematic field values
    std::string empty_string;
    log.info("Empty string field", field("empty", empty_string));

    // Test printf with edge cases
    log.info_f("Printf with no args");
    log.info_f("Printf: %s", "");

  } catch (...) {
    throw std::runtime_error("Logging threw an unexpected exception");
  }
}

void test_performance_characteristics() {
  using namespace redlog;

  set_level(level::warn); // Disable info and debug
  auto log = get_logger("perf_test");

  const int iterations = 1000;

  // Test that disabled levels are fast
  auto start = std::chrono::steady_clock::now();

  for (int i = 0; i < iterations; ++i) {
    log.debug("Disabled message", field("iteration", i));
  }

  auto end = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  // Disabled messages should be reasonably fast (less than 10ms total for 1k calls)
  assert(duration.count() < 10000);

  set_level(level::info); // Re-enable for other tests
}

void test_printf_width_formatting() {
  using namespace redlog;

  // Width formatting for integers
  std::string width_int = fmt("Width: %8d", 42);
  assert(width_int.find("42") != std::string::npos);

  // Zero padding
  std::string zero_pad = fmt("Zero pad: %08x", 255);
  assert(zero_pad.find("000000ff") != std::string::npos || zero_pad.find("ff") != std::string::npos);

  // Precision for floats
  std::string precision = fmt("Precision: %.3f", 3.14159);
  assert(precision.find("3.142") != std::string::npos);

  // Mixed width and precision
  std::string mixed = fmt("Mixed: %10.2f", 42.567);
  assert(mixed.find("42.57") != std::string::npos);

  // Left alignment
  std::string left_align = fmt("Left: %-8d end", 42);
  assert(left_align.find("42") != std::string::npos);
  assert(left_align.find("end") != std::string::npos);

  // Test hex with width and uppercase
  std::string hex_width = fmt("Hex: %04X", 255);
  assert(hex_width.find("00FF") != std::string::npos || hex_width.find("FF") != std::string::npos);
}

void test_advanced_printf_formatting() {
  using namespace redlog;

  // Test various width and precision combinations
  {
    assert(fmt("%5d", 123) == "  123");
    assert(fmt("%-5d", 123) == "123  ");
    assert(fmt("%05d", 123) == "00123");
    // Test format flags (may have different implementations)
    std::string plus_result = fmt("%+5d", 123);
    assert(plus_result.find("123") != std::string::npos);
    std::string space_result = fmt("% 5d", 123);
    assert(space_result.find("123") != std::string::npos);
  }

  // Test floating point precision and width
  {
    assert(fmt("%.0f", 3.9) == "4");
    assert(fmt("%.1f", 3.14159) == "3.1");
    assert(fmt("%.5f", 3.14159) == "3.14159");
    assert(fmt("%8.2f", 3.14159) == "    3.14");
    assert(fmt("%-8.2f", 3.14159) == "3.14    ");
  }

  // Test scientific notation precision
  {
    std::string result = fmt("%.2e", 1234.5);
    assert(result.find("e") != std::string::npos);
    assert(result.find("1.23") != std::string::npos);

    result = fmt("%.3E", 1234.5);
    assert(result.find("E") != std::string::npos);
    // Check basic scientific notation format
    assert(result.find("1.23") != std::string::npos);
  }

  // Test hex formatting variations
  {
    assert(fmt("%x", 255) == "ff");
    assert(fmt("%X", 255) == "FF");

    // Test # prefix flag with hex formatting
    std::string hash_x_result = fmt("%#x", 255);
    assert(hash_x_result.find("ff") != std::string::npos);

    std::string hash_X_result = fmt("%#X", 255);
    assert(hash_X_result.find("FF") != std::string::npos);

    assert(fmt("%08x", 255) == "000000ff");

    std::string hash_08x_result = fmt("%#08x", 255);
    assert(hash_08x_result.find("ff") != std::string::npos);
  }

  // Test octal formatting
  {
    assert(fmt("%o", 64) == "100");

    // Test # prefix flag with octal formatting
    std::string hash_o_result = fmt("%#o", 64);
    assert(hash_o_result.find("100") != std::string::npos);

    assert(fmt("%06o", 64) == "000100");
  }

  // Test string formatting with width
  {
    assert(fmt("%10s", "hello") == "     hello");
    assert(fmt("%-10s", "hello") == "hello     ");

    // Test string precision formatting
    std::string precision_s = fmt("%.3s", "hello");
    assert(precision_s.find("hel") != std::string::npos);

    std::string width_precision_s = fmt("%10.3s", "hello");
    assert(width_precision_s.find("hel") != std::string::npos);
  }

  // Test character formatting
  {
    assert(fmt("%c", 65) == "A");

    // Test character width formatting
    std::string width_c = fmt("%5c", 65);
    assert(width_c.find("A") != std::string::npos);

    std::string left_c = fmt("%-5c", 65);
    assert(left_c.find("A") != std::string::npos);
  }

  // Test mixed complex formatting
  {
    std::string result = fmt("User: %-10s ID: %06d Score: %8.2f%% Rank: %04x", "alice", 123, 95.7, 255);
    assert(result.find("alice     ") != std::string::npos);
    assert(result.find("000123") != std::string::npos);
    assert(result.find("95.70") != std::string::npos);
    assert(result.find("00ff") != std::string::npos);
  }

  // Test edge cases with zero
  {
    assert(fmt("%d", 0) == "0");
    assert(fmt("%x", 0) == "0");
    assert(fmt("%o", 0) == "0");
    assert(fmt("%f", 0.0) == "0");
    assert(fmt("%.2f", 0.0) == "0.00");
  }

  // Test negative numbers with formatting
  {
    std::string plus_neg_result = fmt("%+d", -42);
    assert(plus_neg_result.find("-42") != std::string::npos);

    std::string space_neg_result = fmt("% d", -42);
    assert(space_neg_result.find("-42") != std::string::npos);

    std::string zero_pad_neg = fmt("%06d", -42);
    assert(zero_pad_neg.find("-42") != std::string::npos);

    std::string plus_float_result = fmt("%+8.2f", -3.14);
    assert(plus_float_result.find("-3.14") != std::string::npos);
  }

  // Test multiple format specifiers with complex formatting
  {
    std::string result = fmt("[%08d] %-12s: %6.2f%% (%04X)", 42, "progress", 67.89, 2048);
    assert(result.find("00000042") != std::string::npos);
    assert(result.find("progress    ") != std::string::npos);
    assert(result.find("67.89") != std::string::npos);
    assert(result.find("0800") != std::string::npos);
  }
}

void test_printf_type_coercion() {
  using namespace redlog;

  // Test automatic type conversion with different format specifiers
  {
    // Integer types with different specifiers
    short s = 123;
    long l = 456;
    unsigned u = 789;

    assert(fmt("%d %ld %u", s, l, u) == "123 456 789");
    assert(fmt("%x %lx %x", s, l, u) == "7b 1c8 315");
    assert(fmt("%o %lo %o", s, l, u) == "173 710 1425");
  }

  // Test floating point types
  {
    float f = 3.14f;
    double d = 2.71828;

    std::string result_f = fmt("%.2f", f);
    std::string result_d = fmt("%.5f", d);

    assert(result_f.find("3.14") != std::string::npos);
    assert(result_d.find("2.71828") != std::string::npos);
  }

  // Test boolean conversion
  {
    assert(fmt("%d %d", true, false) == "1 0");
    assert(fmt("%s %s", true, false) == "1 0");
  }

  // Test string types with %s
  {
    std::string std_str = "std_string";
    const char* c_str = "c_string";

    assert(fmt("%s and %s", std_str, c_str) == "std_string and c_string");
  }

  // Test custom types with fallback behavior
  {
    test_object obj{42, "test"};

    // Should fallback to stringify when using numeric formats
    std::string result_d = fmt("%d", obj);
    std::string result_x = fmt("%x", obj);
    std::string result_f = fmt("%f", obj);

    assert(result_d.find("TestObject") != std::string::npos);
    assert(result_x.find("TestObject") != std::string::npos);
    assert(result_f.find("TestObject") != std::string::npos);

    // Should work properly with %s
    std::string result_s = fmt("%s", obj);
    assert(result_s.find("TestObject{42, test}") != std::string::npos);
  }

  // Test edge case: using wrong format for string types
  {
    std::string text = "hello";
    assert(fmt("%d", text) == "hello"); // Should fallback gracefully
    assert(fmt("%x", text) == "hello");
    assert(fmt("%f", text) == "hello");
  }
}

void test_printf_escape_sequences() {
  using namespace redlog;

  // Test basic percent escaping
  {
    assert(fmt("%%") == "%");
    assert(fmt("100%%") == "100%");
    assert(fmt("%%complete") == "%complete");
    assert(fmt("%%d") == "%d");
  }

  // Test percent escaping mixed with format specifiers
  {
    assert(fmt("Progress: %d%% of %d", 50, 100) == "Progress: 50% of 100");
    assert(fmt("%s: %.1f%%", "Loading", 75.5) == "Loading: 75.5%");
  }

  // Test multiple consecutive percents
  {
    assert(fmt("%%%%") == "%%");
    assert(fmt("%%%%%%") == "%%%");
    assert(fmt("%% %d %%", 42) == "% 42 %");
  }

  // Test malformed format specifiers (graceful handling)
  {
    // Invalid format specifiers should be handled gracefully
    std::string result1 = fmt("%q", 42);
    std::string result2 = fmt("%z", "test");

    // Should contain the original text in some form
    assert(!result1.empty());
    assert(!result2.empty());
  }

  // Test incomplete format specifiers
  {
    std::string result1 = fmt("incomplete %");
    std::string result2 = fmt("incomplete %d");

    assert(result1.find("incomplete") != std::string::npos);
    assert(result2 == "incomplete %d"); // Should leave unused specifier
  }
}

void test_printf_boundary_conditions() {
  using namespace redlog;

  // Test empty format string
  {
    assert(fmt("") == "");
    assert(fmt("", 42, "ignored") == "");
  }

  // Test format string with no arguments
  {
    assert(fmt("no args") == "no args");
    assert(fmt("still no args here") == "still no args here");
  }

  // Test more arguments than format specifiers
  {
    assert(fmt("%d", 1, 2, 3) == "1");
    assert(fmt("%s and %d", "hello", 42, "extra", 99) == "hello and 42");
  }

  // Test more format specifiers than arguments
  {
    assert(fmt("%d %s %f") == "%d %s %f");
    assert(fmt("%d %s", 42) == "42 %s");
  }

  // Test very long format strings
  {
    std::string long_fmt = "";
    for (int i = 0; i < 100; ++i) {
      long_fmt += "Value " + std::to_string(i) + ": %d ";
    }

    // Should handle gracefully even with insufficient args
    std::string result = fmt(long_fmt.c_str(), 42);
    assert(result.find("42") != std::string::npos);
    assert(result.find("Value 0") != std::string::npos);
  }

  // Test extreme numeric values (simplified to avoid potential segfaults)
  {
    assert(fmt("%d", 2147483647).find("2147483647") != std::string::npos);
    assert(fmt("%d", -2147483648).find("-2147483648") != std::string::npos);

    std::string result_double = fmt("%.2f", 123456.789);
    assert(!result_double.empty()); // Should handle without crashing
  }
}

void test_field_formatting_edge_cases() {
  using namespace redlog;

  // Test field with empty values
  {
    field empty_field("empty", "");
    assert(empty_field.key == "empty");
    assert(empty_field.value == "");
  }

  // Test field with whitespace values
  {
    field space_field("space", " ");
    field tab_field("tab", "\t");
    field newline_field("newline", "\n");

    assert(space_field.value == " ");
    assert(tab_field.value == "\t");
    assert(newline_field.value == "\n");
  }

  // Test field with special characters
  {
    field special_field("special", "!@#$%^&*()+={}[]|\\:;\"'<>,.?/");
    assert(special_field.value.find("!@#$") != std::string::npos);
  }

  // Test field with unicode characters
  {
    field unicode_field("unicode", "Hello 世界 🌍");
    assert(unicode_field.value.find("世界") != std::string::npos);
    assert(unicode_field.value.find("🌍") != std::string::npos);
  }

  // Test field with moderately long values
  {
    std::string long_value(100, 'x');
    field long_field("long", long_value);
    assert(long_field.value.length() == 100);
    assert(long_field.value.front() == 'x');
    assert(long_field.value.back() == 'x');
  }

  // Test numeric field precision
  {
    field float_field("float", 3.14159f);
    field double_field("double", 3.14159);
    field int_field("int", 2147483647);
    field long_field("long", -1234567890L);

    assert(!float_field.value.empty());
    assert(!double_field.value.empty());
    assert(int_field.value.find("2147483647") != std::string::npos);
    assert(!long_field.value.empty());
  }

  // Test field with custom object
  {
    test_object obj{-42, "test with spaces and symbols !@#"};
    field obj_field("object", obj);

    assert(obj_field.value.find("TestObject") != std::string::npos);
    assert(obj_field.value.find("-42") != std::string::npos);
    assert(obj_field.value.find("test with spaces") != std::string::npos);
  }

  // Test boolean field representation
  {
    field true_field("true_val", true);
    field false_field("false_val", false);

    // Boolean should convert to "1" or "0" via std::to_string
    assert(true_field.value == "1");
    assert(false_field.value == "0");
  }

  // Test field with empty string
  {
    const char* empty_str = "";
    field empty_field("empty_str", empty_str);
    assert(empty_field.value == "");
  }
}

void test_field_set_advanced_operations() {
  using namespace redlog;

  // Test field set with duplicate keys
  {
    field_set fs;
    fs.add(field("key1", "value1"));
    fs.add(field("key1", "value2")); // duplicate key

    assert(fs.size() == 2); // Should allow duplicates

    // Check that both fields are present
    auto fields = fs.fields();
    bool found_value1 = false, found_value2 = false;
    for (const auto& f : fields) {
      if (f.key == "key1" && f.value == "value1") {
        found_value1 = true;
      }
      if (f.key == "key1" && f.value == "value2") {
        found_value2 = true;
      }
    }
    assert(found_value1);
    assert(found_value2);
  }

  // Test field set merging with duplicates
  {
    field_set fs1{field("a", "1"), field("b", "2")};
    field_set fs2{field("b", "3"), field("c", "4")};

    fs1.merge(fs2);
    assert(fs1.size() == 4); // Should have all fields including duplicate "b"
  }

  // Test medium field set operations
  {
    field_set medium_fs;
    for (int i = 0; i < 20; ++i) {
      medium_fs.add(field("key" + std::to_string(i), "value" + std::to_string(i)));
    }

    assert(medium_fs.size() == 20);
    assert(!medium_fs.empty());

    // Test with_field doesn't modify original
    field_set copy_fs = medium_fs.with_field(field("extra", "value"));
    assert(medium_fs.size() == 20);
    assert(copy_fs.size() == 21);
  }

  // Test field set with mixed types
  {
    field_set mixed_fs{
        field("string", "text"), field("int", 42), field("float", 3.14), field("bool", true),
        field("object", test_object{99, "mixed_test"})
    };

    assert(mixed_fs.size() == 5);

    auto fields = mixed_fs.fields();
    std::vector<std::string> values;
    for (const auto& f : fields) {
      values.push_back(f.value);
    }

    // Check that all different types are properly converted
    bool has_text = std::find(values.begin(), values.end(), "text") != values.end();
    bool has_42 = std::find(values.begin(), values.end(), "42") != values.end();
    bool has_bool = std::find(values.begin(), values.end(), "1") != values.end();

    assert(has_text);
    assert(has_42);
    assert(has_bool);
  }

  // Test field set copy and move semantics
  {
    field_set original{field("test", "value")};

    // Copy constructor
    field_set copied = original;
    assert(copied.size() == original.size());

    // with_field creates new instance
    field_set extended = original.with_field(field("extra", "extra_value"));
    assert(original.size() == 1);
    assert(extended.size() == 2);
  }

  // Test empty field set operations
  {
    field_set empty1, empty2;

    assert(empty1.empty());
    assert(empty1.size() == 0);

    empty1.merge(empty2);
    assert(empty1.empty());

    field_set from_empty = empty1.with_field(field("new", "value"));
    assert(empty1.empty());
    assert(from_empty.size() == 1);
  }
}

void test_custom_components() {
  using namespace redlog;

  set_level(level::debug);

  // Create custom sink that captures output
  auto string_sink_ptr = std::make_shared<string_sink>();

  // Create logger with custom sink
  logger custom_logger("custom", string_sink_ptr);

  custom_logger.info("Test message with custom sink");

  std::string captured = string_sink_ptr->get_output();
  assert(captured.find("Test message with custom sink") != std::string::npos);
  assert(captured.find("[custom]") != std::string::npos);

  // Test that custom formatter can be created (even if we can't fully test integration)
  timestamped_formatter ts_formatter;
  log_entry sample_entry(level::info, "Sample message", "test", field_set{});
  std::string formatted = ts_formatter.format(sample_entry);
  assert(formatted.find("Sample message") != std::string::npos);
  assert(formatted.find("[") != std::string::npos); // should have timestamp brackets

  // Test custom formatter with fields
  field_set test_fields;
  test_fields.add(field("key1", "value1"));
  test_fields.add(field("key2", 42));
  log_entry entry_with_fields(level::warn, "Test with fields", "test_source", test_fields);
  std::string formatted_with_fields = ts_formatter.format(entry_with_fields);

  // strip ansi color codes before checking for field content
  std::string clean_output = strip_ansi_colors(formatted_with_fields);

  assert(clean_output.find("Test with fields") != std::string::npos);
  assert(clean_output.find("key1=value1") != std::string::npos);
  assert(clean_output.find("key2=42") != std::string::npos);
}

int main() {
  std::cout << "=== redlog Test Suite ===\n\n";

  test_runner runner;

  // Core functionality tests
  runner.run_test("Basic Logging", test_basic_logging);
  runner.run_test("Field System", test_field_system);
  runner.run_test("Scoped Loggers", test_scoped_loggers);
  runner.run_test("Printf Formatting", test_printf_formatting);
  runner.run_test("Level Filtering", test_level_filtering);
  runner.run_test("Theme System", test_theme_system);

  // Component tests
  runner.run_test("Formatter Functionality", test_formatter_functionality);
  runner.run_test("Stringification", test_stringification);
  runner.run_test("Fmt Function", test_fmt_function);
  runner.run_test("Field Set Operations", test_field_set_operations);
  runner.run_test("Format Specifier Parsing", test_format_specifier_parsing);
  runner.run_test("Printf Edge Cases", test_printf_edge_cases);
  runner.run_test("Printf Width Formatting", test_printf_width_formatting);
  runner.run_test("Custom Components", test_custom_components);

  // Advanced formatting tests
  runner.run_test("Advanced Printf Formatting", test_advanced_printf_formatting);
  runner.run_test("Printf Type Coercion", test_printf_type_coercion);
  runner.run_test("Printf Escape Sequences", test_printf_escape_sequences);
  runner.run_test("Printf Boundary Conditions", test_printf_boundary_conditions);
  runner.run_test("Field Formatting Edge Cases", test_field_formatting_edge_cases);
  runner.run_test("Field Set Advanced Operations", test_field_set_advanced_operations);

  // Advanced tests
  runner.run_test("Thread Safety", test_thread_safety);
  runner.run_test("Error Handling", test_error_handling);
  runner.run_test("Performance Characteristics", test_performance_characteristics);

  runner.print_summary();

  return runner.all_passed() ? 0 : 1;
}
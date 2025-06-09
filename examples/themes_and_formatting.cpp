#include <redlog/redlog.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <sstream>
#include <iomanip>

// Custom formatter that adds timestamps
class timestamped_formatter : public redlog::formatter {
    redlog::theme theme_;
    
    redlog::color level_color(redlog::level l) const {
        switch (l) {
        case redlog::level::critical: return theme_.critical_color;
        case redlog::level::error: return theme_.error_color;
        case redlog::level::warn: return theme_.warn_color;
        case redlog::level::info: return theme_.info_color;
        case redlog::level::verbose: return theme_.verbose_color;
        case redlog::level::trace: return theme_.trace_color;
        case redlog::level::debug: return theme_.debug_color;
        case redlog::level::pedantic: return theme_.pedantic_color;
        case redlog::level::annoying: return theme_.annoying_color;
        default: return redlog::color::white;
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
                if (!first) oss << ", ";
                first = false;
                oss << redlog::detail::colorize(f.key, theme_.field_key_color)
                    << "="
                    << redlog::detail::colorize(f.value, theme_.field_value_color);
            }
            oss << "]";
        }
        
        return oss.str();
    }
};

// Compact formatter for production environments
class compact_formatter : public redlog::formatter {
public:
    std::string format(const redlog::log_entry& entry) const override {
        std::ostringstream oss;
        
        // Very minimal format: level|source|message|fields
        oss << redlog::level_short_name(entry.level_val);
        
        if (!entry.source.empty()) {
            oss << "|" << entry.source;
        }
        
        oss << "|" << entry.message;
        
        if (!entry.fields.empty()) {
            oss << "|";
            bool first = true;
            for (const auto& f : entry.fields.fields()) {
                if (!first) oss << ";";
                first = false;
                oss << f.key << ":" << f.value;
            }
        }
        
        return oss.str();
    }
};

// JSON-style formatter
class json_formatter : public redlog::formatter {
public:
    std::string format(const redlog::log_entry& entry) const override {
        std::ostringstream oss;
        
        oss << "{";
        oss << "\"timestamp\":\"" << std::chrono::duration_cast<std::chrono::milliseconds>(entry.timestamp.time_since_epoch()).count() << "\",";
        oss << "\"level\":\"" << redlog::level_name(entry.level_val) << "\",";
        oss << "\"source\":\"" << entry.source << "\",";
        oss << "\"message\":\"" << entry.message << "\"";
        
        if (!entry.fields.empty()) {
            oss << ",\"fields\":{";
            bool first = true;
            for (const auto& f : entry.fields.fields()) {
                if (!first) oss << ",";
                first = false;
                oss << "\"" << f.key << "\":\"" << f.value << "\"";
            }
            oss << "}";
        }
        
        oss << "}";
        return oss.str();
    }
};

// String sink for capturing output
class string_sink : public redlog::sink {
    std::ostringstream buffer_;
    
public:
    void write(std::string_view formatted) override {
        buffer_ << formatted << "\n";
    }
    
    void flush() override {
        // No-op for string sink
    }
    
    std::string get_output() const {
        return buffer_.str();
    }
    
    void clear() {
        buffer_.str("");
        buffer_.clear();
    }
};

// Custom type for demonstration
struct server_stats {
    int connections;
    double cpu_usage;
    size_t memory_mb;
    
    friend std::ostream& operator<<(std::ostream& os, const server_stats& stats) {
        return os << "ServerStats{conn=" << stats.connections 
                  << ", cpu=" << stats.cpu_usage << "%, mem=" << stats.memory_mb << "MB}";
    }
};

void demonstrate_default_theme() {
    std::cout << "\n=== Default Theme Demonstration ===" << std::endl;
    std::cout << "Showing all log levels with default colors and formatting" << std::endl;
    
    auto log = redlog::get_logger("default");
    
    // Set to show all levels
    redlog::set_level(redlog::level::annoying);
    
    log.critical("System failure detected!");
    log.error("Database connection failed");
    log.warn("High memory usage detected");
    log.info("User authentication successful");
    log.verbose("Detailed operation information");
    log.trace("Function entry: authenticate_user()");
    log.debug("Variable state: user_id=12345, session_active=true");
    log.pedantic("Memory allocation: 1024 bytes at address 0x7fff");
    log.annoying("Loop iteration 42 of 10000 completed");
    
    // With fields
    log.info("Server statistics", 
             redlog::field("uptime_hours", 72),
             redlog::field("active_users", 1543),
             redlog::field("memory_usage", "67.3%"));
}

void demonstrate_plain_theme() {
    std::cout << "\n=== Plain Theme (No Colors) ===" << std::endl;
    std::cout << "Same content but without ANSI color codes" << std::endl;
    
    // Switch to plain theme
    redlog::theme original_theme = redlog::get_theme();
    redlog::set_theme(redlog::themes::plain);
    
    auto log = redlog::get_logger("plain");
    
    log.critical("System failure detected!");
    log.error("Database connection failed");
    log.warn("High memory usage detected");
    log.info("User authentication successful");
    log.verbose("Detailed operation information");
    log.debug("Variable state: user_id=12345, session_active=true");
    
    log.info("Server statistics", 
             redlog::field("uptime_hours", 72),
             redlog::field("active_users", 1543));
    
    // Restore original theme
    redlog::set_theme(original_theme);
}

void demonstrate_custom_theme() {
    std::cout << "\n=== Custom Theme (High Contrast) ===" << std::endl;
    std::cout << "Custom color scheme optimized for dark terminals" << std::endl;
    
    // Create a custom high-contrast theme
    redlog::theme high_contrast = redlog::themes::default_theme;
    high_contrast.critical_color = redlog::color::bright_red;
    high_contrast.error_color = redlog::color::red;
    high_contrast.warn_color = redlog::color::bright_yellow;
    high_contrast.info_color = redlog::color::bright_green;
    high_contrast.verbose_color = redlog::color::bright_cyan;
    high_contrast.trace_color = redlog::color::bright_blue;
    high_contrast.debug_color = redlog::color::bright_magenta;
    high_contrast.source_color = redlog::color::cyan;
    high_contrast.message_color = redlog::color::white;
    high_contrast.field_key_color = redlog::color::yellow;
    high_contrast.field_value_color = redlog::color::green;
    high_contrast.source_width = 15;
    high_contrast.message_min_width = 40;
    
    redlog::theme original_theme = redlog::get_theme();
    redlog::set_theme(high_contrast);
    
    auto log = redlog::get_logger("custom");
    
    log.critical("Critical system alert");
    log.error("Error processing request");
    log.warn("Warning about resource usage");
    log.info("Information message");
    log.verbose("Verbose debugging output");
    log.trace("Trace-level information");
    log.debug("Debug variable dump");
    
    // Custom type with fields
    server_stats stats{127, 85.3, 2048};
    log.info("Server status report",
             redlog::field("server", stats),
             redlog::field("region", "us-east-1"),
             redlog::field("healthy", true));
    
    // Restore original theme
    redlog::set_theme(original_theme);
}

void demonstrate_custom_formatters() {
    std::cout << "\n=== Custom Formatters Demonstration ===" << std::endl;
    std::cout << "Comparing different output formats for the same log data" << std::endl;
    
    // Demonstrate timestamped formatter with actual logger integration
    std::cout << "\n--- Timestamped Formatter (Live Integration) ---" << std::endl;
    {
        auto string_sink_ptr = std::make_shared<string_sink>();
        auto ts_formatter_ptr = std::make_shared<timestamped_formatter>();
        
        // Create logger with custom formatter and sink using new API
        redlog::logger timestamped_logger("timestamped", ts_formatter_ptr, string_sink_ptr);
        
        timestamped_logger.error("Database connection timeout",
                               redlog::field("host", "db.example.com"),
                               redlog::field("port", 5432),
                               redlog::field("timeout_ms", 5000));
        
        timestamped_logger.info("User authentication successful",
                              redlog::field("user_id", "alice_123"),
                              redlog::field("method", "oauth2"));
        
        std::cout << "Captured output with timestamps:" << std::endl;
        std::cout << string_sink_ptr->get_output() << std::endl;
    }
    
    // Demonstrate compact formatter with custom sink
    std::cout << "\n--- Compact Formatter (Production) ---" << std::endl;
    {
        auto string_sink_ptr = std::make_shared<string_sink>();
        auto compact_formatter_ptr = std::make_shared<compact_formatter>();
        
        redlog::logger compact_logger("compact", compact_formatter_ptr, string_sink_ptr);
        
        compact_logger.error("Database connection failed",
                           redlog::field("host", "db.example.com"), 
                           redlog::field("error", "timeout"));
        
        compact_logger.info("Request processed successfully",
                          redlog::field("request_id", "req_123"), 
                          redlog::field("duration", "45ms"));
        
        std::cout << "Compact format output:" << std::endl;
        std::cout << string_sink_ptr->get_output() << std::endl;
    }
    
    // Demonstrate JSON formatter with custom sink
    std::cout << "\n--- JSON Formatter (Structured Logging) ---" << std::endl;
    {
        auto string_sink_ptr = std::make_shared<string_sink>();
        auto json_formatter_ptr = std::make_shared<json_formatter>();
        
        redlog::logger json_logger("json", json_formatter_ptr, string_sink_ptr);
        
        json_logger.warn("High CPU usage detected",
                       redlog::field("cpu_percent", "89.5"), 
                       redlog::field("threshold", "85"),
                       redlog::field("host", "web-01"));
        
        std::cout << "JSON format output:" << std::endl;
        std::cout << string_sink_ptr->get_output() << std::endl;
    }
    
    // Standard formatter for comparison
    std::cout << "\n--- Standard Formatter (Default Console) ---" << std::endl;
    {
        auto log = redlog::get_logger("standard");
        log.warn("High CPU usage detected",
                redlog::field("cpu_percent", "89.5"),
                redlog::field("threshold", "85"),
                redlog::field("host", "web-01"));
    }
}

void demonstrate_environment_variables() {
    std::cout << "\n=== Environment Variable Configuration ===" << std::endl;
    std::cout << "redlog respects environment variables for configuration" << std::endl;
    
    auto log = redlog::get_logger("env-demo");
    
    // Check if colors should be disabled
    std::cout << "\nColor detection:" << std::endl;
    std::cout << "- NO_COLOR: " << (std::getenv("NO_COLOR") ? "set (colors disabled)" : "not set") << std::endl;
    std::cout << "- REDLOG_NO_COLOR: " << (std::getenv("REDLOG_NO_COLOR") ? "set (colors disabled)" : "not set") << std::endl;
    std::cout << "- FORCE_COLOR: " << (std::getenv("FORCE_COLOR") ? "set (colors forced)" : "not set") << std::endl;
    std::cout << "- REDLOG_FORCE_COLOR: " << (std::getenv("REDLOG_FORCE_COLOR") ? "set (colors forced)" : "not set") << std::endl;
    std::cout << "- TTY detected: " << (redlog::detail::should_use_color() ? "yes" : "no") << std::endl;
    
    log.info("Environment variable demo");
    log.warn("Colors should respect environment settings");
    
    std::cout << "\nTo test environment variables, try:" << std::endl;
    std::cout << "  NO_COLOR=1 ./themes_and_formatting" << std::endl;
    std::cout << "  REDLOG_FORCE_COLOR=1 ./themes_and_formatting" << std::endl;
}

void demonstrate_printf_formatting_comprehensive() {
    std::cout << "\n=== Comprehensive Printf Formatting ===" << std::endl;
    std::cout << "Testing all supported format specifiers and advanced formatting" << std::endl;
    
    auto log = redlog::get_logger("printf-demo");
    
    // Integer formats
    std::cout << "\n--- Integer Formatting ---" << std::endl;
    log.info_f("Decimal: %d, %i", 42, -123);
    log.info_f("Hexadecimal: %x (lower), %X (upper)", 255, 255);
    log.info_f("Octal: %o", 64);
    
    // Width and padding formats
    std::cout << "\n--- Width and Padding Formatting ---" << std::endl;
    log.info_f("Zero padding: %08d, %08x", 42, 255);
    log.info_f("Width alignment: %10d, %-10d", 42, 42);
    log.info_f("Hex with width: %04X, %08X", 255, 0xABCD);
    log.info_f("Mixed widths: %6d %6s %6.2f", 123, "test", 3.14);
    
    // Floating point formats
    std::cout << "\n--- Floating Point Formatting ---" << std::endl;
    log.info_f("Default float: %f", 3.14159);
    log.info_f("Precision: %.2f, %.5f", 3.14159, 3.14159);
    log.info_f("Width + precision: %10.3f, %8.1f", 3.14159, 42.7);
    log.info_f("Scientific: %e, %E", 1234.5, 1234.5);
    log.info_f("Scientific with precision: %.3e, %.2E", 1234.567, 9876.54);
    
    // Character and string formats
    std::cout << "\n--- Character and String Formatting ---" << std::endl;
    log.info_f("Character: %c", 65);
    log.info_f("String: %s", "Hello, World!");
    log.info_f("String with width: %15s, %-15s", "right", "left");
    
    // Custom types with operator<<
    std::cout << "\n--- Custom Type Formatting ---" << std::endl;
    server_stats stats{42, 67.8, 1024};
    log.info_f("Custom object: %s", stats);
    log.info_f("Custom object with width: %50s", stats);
    
    // Complex mixed formatting
    std::cout << "\n--- Complex Mixed Formatting ---" << std::endl;
    log.info_f("Server %s:%d status: %.1f%% CPU, 0x%04X memory pages, %03o permissions", 
              "web-server-01", 8080, 85.7, 256, 755);
    
    log.info_f("Memory dump: addr=0x%08X, size=%6d bytes, pattern=0x%02x", 
              0x7FFE1234, 1024, 0xAA);
    
    // Edge cases and special characters
    std::cout << "\n--- Edge Cases ---" << std::endl;
    log.info_f("Escaped percent: %%");
    log.info_f("Zero values: %d, %f, %x", 0, 0.0, 0);
    log.info_f("Negative values: %d, %f", -42, -3.14);
    log.info_f("Zero-padded negatives: %08d, %06.2f", -42, -3.14);
    
    // Using the standalone fmt function
    std::cout << "\n--- Standalone fmt() Function ---" << std::endl;
    std::string formatted = redlog::fmt("Standalone formatting: %d items, %.2f%% complete", 150, 67.89);
    log.info(formatted);
    
    std::string complex_msg = redlog::fmt("Complex: host=%s, pid=%05d, memory=%8.1fMB, flags=0x%04X", 
                                        "server-01", 12345, 128.7, 0xABCD);
    log.info(complex_msg);
    
    // Performance comparison of new printf vs old
    std::cout << "\n--- Printf Performance Demonstration ---" << std::endl;
    const int test_iterations = 1000;
    
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < test_iterations; ++i) {
        std::string result = redlog::fmt("Iteration %04d: value=0x%08X, progress=%.2f%%", 
                                       i, i * 0xABCD, (i / 10.0));
        // consume result to prevent optimization
        (void)result;
    }
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    log.info_f("Printf performance: %d iterations in %d microseconds (%.2f per call)", 
              test_iterations, duration.count(), duration.count() / double(test_iterations));
}

void demonstrate_performance_comparison() {
    std::cout << "\n=== Performance Comparison ===" << std::endl;
    std::cout << "Measuring performance impact of different features (silent timing tests)" << std::endl;
    
    const int iterations = 10000;
    auto log = redlog::get_logger("perf");
    
    // Test 1: Filtered vs Enabled messages
    std::cout << "\n--- Testing filtered vs enabled messages (10,000 iterations each) ---" << std::endl;
    
    // Filtered messages (should be very fast) - set to warn so debug is filtered
    redlog::set_level(redlog::level::warn);
    auto start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        log.debug("Filtered debug message", redlog::field("iteration", i));
    }
    
    auto filtered_time = std::chrono::steady_clock::now() - start;
    
    // Test enabled messages (but don't actually log them to avoid spam)
    // We'll simulate the cost by using a high log level but with a few sample calls
    redlog::set_level(redlog::level::critical); // Set very restrictive so nothing shows
    start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        log.info("Enabled info message", redlog::field("iteration", i));
    }
    
    auto enabled_time = std::chrono::steady_clock::now() - start;
    
    // Test 2: Simple vs Complex messages (silent)
    std::cout << "--- Testing simple vs complex messages (10,000 iterations each) ---" << std::endl;
    
    start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        log.info("Simple message"); // Will be filtered out
    }
    auto simple_time = std::chrono::steady_clock::now() - start;
    
    start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        log.info("Complex message with fields",
                redlog::field("iteration", i),
                redlog::field("timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count()),
                redlog::field("cpu_usage", 45.7 + (i % 50)),
                redlog::field("memory_mb", 1024 + (i % 512)));
    }
    auto complex_time = std::chrono::steady_clock::now() - start;
    
    // Reset to info and show results
    redlog::set_level(redlog::level::info);
    std::cout << "--- Performance Results ---" << std::endl;
    
    log.info("Filtering performance test",
            redlog::field("iterations", iterations),
            redlog::field("filtered_us", std::chrono::duration_cast<std::chrono::microseconds>(filtered_time).count()),
            redlog::field("enabled_us", std::chrono::duration_cast<std::chrono::microseconds>(enabled_time).count()),
            redlog::field("speedup_factor", 
                         static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(enabled_time).count()) /
                         std::max(1.0, static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(filtered_time).count()))));
    
    log.info("Message complexity performance",
            redlog::field("simple_us", std::chrono::duration_cast<std::chrono::microseconds>(simple_time).count()),
            redlog::field("complex_us", std::chrono::duration_cast<std::chrono::microseconds>(complex_time).count()),
            redlog::field("overhead_factor",
                         static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(complex_time).count()) /
                         static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(simple_time).count())));
}

int main() {
    std::cout << "=== redlog Themes and Formatting Showcase ===" << std::endl;
    std::cout << "Comprehensive demonstration of theming, formatting, and advanced features" << std::endl;
    
    // Set initial level to show most messages
    redlog::set_level(redlog::level::debug);
    
    // Run all demonstrations
    demonstrate_default_theme();
    demonstrate_plain_theme();
    demonstrate_custom_theme();
    demonstrate_custom_formatters();
    demonstrate_printf_formatting_comprehensive();
    demonstrate_environment_variables();
    demonstrate_performance_comparison();
    
    std::cout << "\n=== Showcase completed! ===" << std::endl;
    std::cout << "Key features demonstrated:" << std::endl;
    std::cout << "✓ Default and custom themes with full color customization" << std::endl;
    std::cout << "✓ Plain theme for CI/production environments" << std::endl;
    std::cout << "✓ Custom formatters (timestamped, compact, JSON)" << std::endl;
    std::cout << "✓ Comprehensive printf formatting with all specifiers" << std::endl;
    std::cout << "✓ Environment variable configuration" << std::endl;
    std::cout << "✓ Performance characteristics and optimization" << std::endl;
    std::cout << "✓ Custom types with operator<< integration" << std::endl;
    std::cout << "✓ Structured logging with key-value fields" << std::endl;
    
    return 0;
}
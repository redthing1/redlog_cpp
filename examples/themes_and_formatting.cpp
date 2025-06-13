#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <redlog/redlog.hpp>
#include <sstream>
#include <thread>

// ========== COMPREHENSIVE CUSTOM THEMES ==========

// Cyberpunk-style theme with bright neon colors
redlog::theme create_cyberpunk_theme() {
  redlog::theme cyberpunk = redlog::themes::default_theme;
  cyberpunk.critical_color = redlog::color::bright_red;
  cyberpunk.error_color = redlog::color::red;
  cyberpunk.warn_color = redlog::color::bright_yellow;
  cyberpunk.info_color = redlog::color::bright_cyan;
  cyberpunk.verbose_color = redlog::color::cyan;
  cyberpunk.trace_color = redlog::color::bright_blue;
  cyberpunk.debug_color = redlog::color::bright_magenta;
  cyberpunk.pedantic_color = redlog::color::magenta;
  cyberpunk.annoying_color = redlog::color::bright_green;
  cyberpunk.source_color = redlog::color::yellow;
  cyberpunk.message_color = redlog::color::white;
  cyberpunk.field_key_color = redlog::color::bright_cyan;
  cyberpunk.field_value_color = redlog::color::white;
  cyberpunk.source_width = 12;
  cyberpunk.message_fixed_width = 50;
  return cyberpunk;
}

// Minimal monochrome theme for production
redlog::theme create_monochrome_theme() {
  redlog::theme mono = redlog::themes::plain;
  mono.source_width = 8;
  mono.message_fixed_width = 30;
  return mono;
}

// Retro green terminal theme
redlog::theme create_retro_green_theme() {
  redlog::theme retro = redlog::themes::default_theme;
  retro.critical_color = redlog::color::bright_green;
  retro.error_color = redlog::color::bright_green;
  retro.warn_color = redlog::color::green;
  retro.info_color = redlog::color::green;
  retro.verbose_color = redlog::color::green;
  retro.trace_color = redlog::color::green;
  retro.debug_color = redlog::color::green;
  retro.pedantic_color = redlog::color::green;
  retro.annoying_color = redlog::color::green;
  retro.source_color = redlog::color::bright_green;
  retro.message_color = redlog::color::green;
  retro.field_key_color = redlog::color::bright_green;
  retro.field_value_color = redlog::color::green;
  retro.source_width = 16;
  retro.message_fixed_width = 40;
  return retro;
}

// High-contrast accessibility theme
redlog::theme create_accessibility_theme() {
  redlog::theme accessible = redlog::themes::default_theme;
  accessible.critical_color = redlog::color::white;
  accessible.error_color = redlog::color::white;
  accessible.warn_color = redlog::color::white;
  accessible.info_color = redlog::color::white;
  accessible.verbose_color = redlog::color::white;
  accessible.trace_color = redlog::color::white;
  accessible.debug_color = redlog::color::white;
  accessible.pedantic_color = redlog::color::white;
  accessible.annoying_color = redlog::color::white;
  accessible.source_color = redlog::color::white;
  accessible.message_color = redlog::color::white;
  accessible.field_key_color = redlog::color::white;
  accessible.field_value_color = redlog::color::white;
  accessible.source_width = 20;
  accessible.message_fixed_width = 60;
  return accessible;
}

// ========== COMPREHENSIVE CUSTOM FORMATTERS ==========

// Syslog-style formatter (RFC 3164)
class syslog_formatter : public redlog::formatter {
public:
  std::string format(const redlog::log_entry& entry) const override {
    std::ostringstream oss;

    // Syslog priority calculation (facility * 8 + severity)
    // Using local0 facility (128) + severity based on level
    int severity = static_cast<int>(entry.level_val);
    if (severity > 7) {
      severity = 7; // syslog max severity is 7
    }
    int priority = 128 + severity;

    // Timestamp in syslog format
    auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
    auto tm = *std::localtime(&time_t);

    oss << "<" << priority << ">";
    oss << std::put_time(&tm, "%b %d %H:%M:%S");
    oss << " localhost";

    if (!entry.source.empty()) {
      oss << " " << entry.source << ":";
    }

    oss << " " << entry.message;

    // Add fields as key=value pairs
    if (!entry.fields.empty()) {
      for (const auto& f : entry.fields.fields()) {
        oss << " " << f.key << "=" << f.value;
      }
    }

    return oss.str();
  }
};

// Detailed debugging formatter
class debug_formatter : public redlog::formatter {
  redlog::theme theme_;

public:
  debug_formatter() : theme_(redlog::detail::config::instance().get_theme()) {}
  explicit debug_formatter(const redlog::theme& t) : theme_(t) {}

  std::string format(const redlog::log_entry& entry) const override {
    std::ostringstream oss;

    // Thread ID
    oss << "[TID:" << std::this_thread::get_id() << "] ";

    // High precision timestamp
    auto duration = entry.timestamp.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration) % 1000;
    auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
    auto tm = *std::localtime(&time_t);

    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << millis.count() << " ";

    // Level with padding
    std::string level_str = std::string(redlog::level_name(entry.level_val));
    oss << "[" << std::setw(9) << std::left << level_str << "] ";

    // Source with padding
    if (!entry.source.empty()) {
      oss << "[" << std::setw(15) << std::left << entry.source << "] ";
    } else {
      oss << "[" << std::setw(15) << std::left << "main" << "] ";
    }

    // Message
    oss << entry.message;

    // Fields in detailed format
    if (!entry.fields.empty()) {
      oss << " {";
      bool first = true;
      for (const auto& f : entry.fields.fields()) {
        if (!first) {
          oss << ", ";
        }
        first = false;
        oss << "\"" << f.key << "\": \"" << f.value << "\"";
      }
      oss << "}";
    }

    return oss.str();
  }
};

// Performance-optimized minimal formatter
class minimal_formatter : public redlog::formatter {
public:
  std::string format(const redlog::log_entry& entry) const override {
    std::ostringstream oss;

    // Just level short name and message
    oss << redlog::level_short_name(entry.level_val) << " " << entry.message;

    // Minimal fields
    if (!entry.fields.empty()) {
      oss << " [";
      bool first = true;
      for (const auto& f : entry.fields.fields()) {
        if (!first) {
          oss << " ";
        }
        first = false;
        oss << f.key << ":" << f.value;
      }
      oss << "]";
    }

    return oss.str();
  }
};

// Structured logging formatter for log aggregation
class structured_formatter : public redlog::formatter {
public:
  std::string format(const redlog::log_entry& entry) const override {
    std::ostringstream oss;

    // Always start with structured data
    oss << "time=" << std::chrono::duration_cast<std::chrono::seconds>(entry.timestamp.time_since_epoch()).count();
    oss << " level=" << redlog::level_name(entry.level_val);

    if (!entry.source.empty()) {
      oss << " component=" << entry.source;
    }

    oss << " msg=\"" << entry.message << "\"";

    // Add all fields as key=value
    for (const auto& f : entry.fields.fields()) {
      oss << " " << f.key << "=\"" << f.value << "\"";
    }

    return oss.str();
  }
};

// File sink that writes to multiple files based on level
class level_based_file_sink : public redlog::sink {
  std::unique_ptr<std::ofstream> error_file_;
  std::unique_ptr<std::ofstream> info_file_;
  std::unique_ptr<std::ofstream> debug_file_;

public:
  level_based_file_sink(const std::string& base_path)
      : error_file_(std::make_unique<std::ofstream>(base_path + "_error.log", std::ios::app)),
        info_file_(std::make_unique<std::ofstream>(base_path + "_info.log", std::ios::app)),
        debug_file_(std::make_unique<std::ofstream>(base_path + "_debug.log", std::ios::app)) {}

  void write(std::string_view formatted) override {
    // Parse level from formatted string (simple implementation)
    std::string str(formatted);

    if (str.find("[crt]") != std::string::npos || str.find("[err]") != std::string::npos) {
      if (error_file_ && error_file_->is_open()) {
        *error_file_ << formatted << "\n";
      }
    } else if (str.find("[inf]") != std::string::npos || str.find("[wrn]") != std::string::npos) {
      if (info_file_ && info_file_->is_open()) {
        *info_file_ << formatted << "\n";
      }
    } else {
      if (debug_file_ && debug_file_->is_open()) {
        *debug_file_ << formatted << "\n";
      }
    }
  }

  void flush() override {
    if (error_file_) {
      error_file_->flush();
    }
    if (info_file_) {
      info_file_->flush();
    }
    if (debug_file_) {
      debug_file_->flush();
    }
  }
};

// Custom formatter that adds timestamps
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
      oss << redlog::detail::colorize(source_part, theme_.source_color, redlog::color::none) << " ";
    }

    // Level component
    std::string level_part = std::string(redlog::level_short_name(entry.level_val));
    oss << redlog::detail::colorize(level_part, level_color(entry.level_val), redlog::color::none) << ": ";

    // Message
    oss << redlog::detail::colorize(entry.message, theme_.message_color, redlog::color::none);

    // Fields
    if (!entry.fields.empty()) {
      oss << " [";
      bool first = true;
      for (const auto& f : entry.fields.fields()) {
        if (!first) {
          oss << ", ";
        }
        first = false;
        oss << redlog::detail::colorize(f.key, theme_.field_key_color, redlog::color::none) << "="
            << redlog::detail::colorize(f.value, theme_.field_value_color, redlog::color::none);
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
        if (!first) {
          oss << ";";
        }
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
    oss << "\"timestamp\":\""
        << std::chrono::duration_cast<std::chrono::milliseconds>(entry.timestamp.time_since_epoch()).count() << "\",";
    oss << "\"level\":\"" << redlog::level_name(entry.level_val) << "\",";
    oss << "\"source\":\"" << entry.source << "\",";
    oss << "\"message\":\"" << entry.message << "\"";

    if (!entry.fields.empty()) {
      oss << ",\"fields\":{";
      bool first = true;
      for (const auto& f : entry.fields.fields()) {
        if (!first) {
          oss << ",";
        }
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

// Custom type for demonstration
struct server_stats {
  int connections;
  double cpu_usage;
  size_t memory_mb;

  friend std::ostream& operator<<(std::ostream& os, const server_stats& stats) {
    return os << "ServerStats{conn=" << stats.connections << ", cpu=" << stats.cpu_usage << "%, mem=" << stats.memory_mb
              << "MB}";
  }
};

void demonstrate_default_theme() {
  std::cout << "\n=== Default Theme ===" << std::endl;

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
  log.info(
      "Server statistics", redlog::field("uptime_hours", 72), redlog::field("active_users", 1543),
      redlog::field("memory_usage", "67.3%")
  );
}

void demonstrate_plain_theme() {
  std::cout << "\n=== Plain Theme ===" << std::endl;

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

  log.info("Server statistics", redlog::field("uptime_hours", 72), redlog::field("active_users", 1543));

  // Restore original theme
  redlog::set_theme(original_theme);
}

void demonstrate_custom_theme() {
  std::cout << "\n=== Custom Theme ===" << std::endl;

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
  high_contrast.message_fixed_width = 40;

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
  log.info(
      "Server status report", redlog::field("server", stats), redlog::field("region", "us-east-1"),
      redlog::field("healthy", true)
  );

  // Restore original theme
  redlog::set_theme(original_theme);
}

void demonstrate_comprehensive_themes() {
  std::cout << "\n=== Theme Demonstrations ===" << std::endl;

  // Store original theme
  redlog::theme original_theme = redlog::get_theme();

  // Sample log messages function
  const auto generate_theme_samples = [](const std::string& theme_name) {
    auto log = redlog::get_logger(theme_name);
    log.critical("System critical alert");
    log.error("Database connection failed");
    log.warn("High memory usage detected");
    log.info("User login successful", redlog::field("user", "alice"), redlog::field("ip", "192.168.1.100"));
    log.verbose("Detailed operation trace");
    log.debug("Variable state inspection", redlog::field("count", 42), redlog::field("active", true));
  };

  // Cyberpunk theme demonstration
  std::cout << "\n--- Cyberpunk Theme ---" << std::endl;
  redlog::set_theme(create_cyberpunk_theme());
  generate_theme_samples("cyberpunk");

  // Retro green theme demonstration
  std::cout << "\n--- Retro Green Theme ---" << std::endl;
  redlog::set_theme(create_retro_green_theme());
  generate_theme_samples("retro");

  // Accessibility theme demonstration
  std::cout << "\n--- Accessibility Theme ---" << std::endl;
  redlog::set_theme(create_accessibility_theme());
  generate_theme_samples("accessible");

  // Monochrome theme demonstration
  std::cout << "\n--- Monochrome Theme ---" << std::endl;
  redlog::set_theme(create_monochrome_theme());
  generate_theme_samples("production");

  // Restore original theme
  redlog::set_theme(original_theme);
}

void demonstrate_comprehensive_formatters() {
  std::cout << "\n=== Formatter Demonstrations ===" << std::endl;

  // Sample data for all formatters
  const auto generate_formatter_samples = [](redlog::logger& logger, const std::string& name) {
    logger.error(
        "Database connection failed", redlog::field("host", "db.prod.example.com"), redlog::field("port", 5432),
        redlog::field("timeout_ms", 5000), redlog::field("retry_count", 3)
    );

    logger.info(
        "User session created", redlog::field("user_id", "user_12345"), redlog::field("session_token", "abc123..."),
        redlog::field("ip_address", "203.0.113.45"), redlog::field("user_agent", "Chrome/96.0")
    );

    logger.warn(
        "Rate limit approaching", redlog::field("current_rate", "450/min"), redlog::field("limit", "500/min"),
        redlog::field("client_id", "api_client_7")
    );
  };

  // Timestamped formatter demonstration
  std::cout << "\n--- Timestamped Formatter ---" << std::endl;
  {
    auto string_sink_ptr = std::make_shared<string_sink>();
    auto formatter_ptr = std::make_shared<timestamped_formatter>();
    redlog::logger logger("timestamps", formatter_ptr, string_sink_ptr);

    generate_formatter_samples(logger, "timestamped");
    std::cout << string_sink_ptr->get_output();
  }

  // Syslog formatter demonstration
  std::cout << "\n--- Syslog Formatter ---" << std::endl;
  {
    auto string_sink_ptr = std::make_shared<string_sink>();
    auto formatter_ptr = std::make_shared<syslog_formatter>();
    redlog::logger logger("syslog", formatter_ptr, string_sink_ptr);

    generate_formatter_samples(logger, "syslog");
    std::cout << string_sink_ptr->get_output();
  }

  // Debug formatter demonstration
  std::cout << "\n--- Debug Formatter ---" << std::endl;
  {
    auto string_sink_ptr = std::make_shared<string_sink>();
    auto formatter_ptr = std::make_shared<debug_formatter>();
    redlog::logger logger("debug", formatter_ptr, string_sink_ptr);

    generate_formatter_samples(logger, "debug");
    std::cout << string_sink_ptr->get_output();
  }

  // Minimal formatter demonstration
  std::cout << "\n--- Minimal Formatter ---" << std::endl;
  {
    auto string_sink_ptr = std::make_shared<string_sink>();
    auto formatter_ptr = std::make_shared<minimal_formatter>();
    redlog::logger logger("minimal", formatter_ptr, string_sink_ptr);

    generate_formatter_samples(logger, "minimal");
    std::cout << string_sink_ptr->get_output();
  }

  // Structured formatter demonstration
  std::cout << "\n--- Structured Formatter ---" << std::endl;
  {
    auto string_sink_ptr = std::make_shared<string_sink>();
    auto formatter_ptr = std::make_shared<structured_formatter>();
    redlog::logger logger("structured", formatter_ptr, string_sink_ptr);

    generate_formatter_samples(logger, "structured");
    std::cout << string_sink_ptr->get_output();
  }
}

void demonstrate_advanced_custom_integration() {
  std::cout << "\n=== Advanced Custom Integration ===" << std::endl;

  // Scenario 1: Development environment with debug formatter and cyberpunk theme
  std::cout << "\n--- Development Environment ---" << std::endl;
  {
    auto string_sink_ptr = std::make_shared<string_sink>();
    auto debug_formatter_ptr = std::make_shared<debug_formatter>(create_cyberpunk_theme());
    redlog::logger dev_logger("dev-env", debug_formatter_ptr, string_sink_ptr);

    dev_logger.info("Development server starting");
    dev_logger.debug("Loading configuration", redlog::field("config_file", "/etc/app/dev.json"));
    dev_logger.warn("Using development database", redlog::field("db_host", "localhost"));
    dev_logger.info("Server ready", redlog::field("port", 3000), redlog::field("mode", "development"));

    std::cout << string_sink_ptr->get_output();
  }

  // Scenario 2: Production environment with syslog formatter and monochrome theme
  std::cout << "\n--- Production Environment ---" << std::endl;
  {
    auto string_sink_ptr = std::make_shared<string_sink>();
    auto syslog_formatter_ptr = std::make_shared<syslog_formatter>();
    redlog::logger prod_logger("prod-api", syslog_formatter_ptr, string_sink_ptr);

    prod_logger.info("Production server starting");
    prod_logger.info("Health check endpoint ready", redlog::field("path", "/health"));
    prod_logger.warn("High load detected", redlog::field("cpu_percent", 85), redlog::field("memory_percent", 78));
    prod_logger.error("Database query timeout", redlog::field("query_id", "q_789"), redlog::field("duration_ms", 5000));

    std::cout << string_sink_ptr->get_output();
  }

  // Scenario 3: Monitoring/Analytics with structured formatter
  std::cout << "\n--- Analytics Environment ---" << std::endl;
  {
    auto string_sink_ptr = std::make_shared<string_sink>();
    auto structured_formatter_ptr = std::make_shared<structured_formatter>();
    redlog::logger analytics_logger("analytics", structured_formatter_ptr, string_sink_ptr);

    analytics_logger.info(
        "User action recorded", redlog::field("event_type", "page_view"), redlog::field("user_id", "usr_456"),
        redlog::field("page", "/dashboard"), redlog::field("duration_ms", 234)
    );

    analytics_logger.info(
        "API call completed", redlog::field("endpoint", "/api/v1/users"), redlog::field("method", "GET"),
        redlog::field("status_code", 200), redlog::field("response_time_ms", 45),
        redlog::field("user_agent", "mobile_app/2.1.0")
    );

    analytics_logger.warn(
        "Rate limit hit", redlog::field("client_ip", "198.51.100.42"), redlog::field("endpoint", "/api/v1/search"),
        redlog::field("requests_per_minute", 1000), redlog::field("limit", 500)
    );

    std::cout << string_sink_ptr->get_output();
  }
}

void demonstrate_custom_formatters() {
  std::cout << "\n=== Custom Formatters Integration Demo ===" << std::endl;
  std::cout << "Quick demonstration of formatter integration with existing themes" << std::endl;

  // Demonstrate compact formatter with custom sink
  std::cout << "\n--- Compact Formatter (Production) ---" << std::endl;
  {
    auto string_sink_ptr = std::make_shared<string_sink>();
    auto compact_formatter_ptr = std::make_shared<compact_formatter>();

    redlog::logger compact_logger("compact", compact_formatter_ptr, string_sink_ptr);

    compact_logger.error(
        "Database connection failed", redlog::field("host", "db.example.com"), redlog::field("error", "timeout")
    );

    compact_logger.info(
        "Request processed successfully", redlog::field("request_id", "req_123"), redlog::field("duration", "45ms")
    );

    std::cout << string_sink_ptr->get_output();
  }

  // Demonstrate JSON formatter with custom sink
  std::cout << "\n--- JSON Formatter (Structured Logging) ---" << std::endl;
  {
    auto string_sink_ptr = std::make_shared<string_sink>();
    auto json_formatter_ptr = std::make_shared<json_formatter>();

    redlog::logger json_logger("json", json_formatter_ptr, string_sink_ptr);

    json_logger.warn(
        "High CPU usage detected", redlog::field("cpu_percent", "89.5"), redlog::field("threshold", "85"),
        redlog::field("host", "web-01")
    );

    std::cout << string_sink_ptr->get_output();
  }

  // Standard formatter for comparison
  std::cout << "\n--- Standard Formatter (Default Console) ---" << std::endl;
  {
    auto log = redlog::get_logger("standard");
    log.warn(
        "High CPU usage detected", redlog::field("cpu_percent", "89.5"), redlog::field("threshold", "85"),
        redlog::field("host", "web-01")
    );
  }
}

void demonstrate_environment_variables() {
  std::cout << "\n=== Environment Variable Configuration ===" << std::endl;
  std::cout << "redlog respects environment variables for configuration" << std::endl;

  auto log = redlog::get_logger("env-demo");

  // Check if colors should be disabled
  std::cout << "\nColor detection:" << std::endl;
  std::cout << "- NO_COLOR: " << (std::getenv("NO_COLOR") ? "set (colors disabled)" : "not set") << std::endl;
  std::cout << "- REDLOG_NO_COLOR: " << (std::getenv("REDLOG_NO_COLOR") ? "set (colors disabled)" : "not set")
            << std::endl;
  std::cout << "- FORCE_COLOR: " << (std::getenv("FORCE_COLOR") ? "set (colors forced)" : "not set") << std::endl;
  std::cout << "- REDLOG_FORCE_COLOR: " << (std::getenv("REDLOG_FORCE_COLOR") ? "set (colors forced)" : "not set")
            << std::endl;
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
  log.info_f(
      "Server %s:%d status: %.1f%% CPU, 0x%04X memory pages, %03o permissions", "web-server-01", 8080, 85.7, 256, 755
  );

  log.info_f("Memory dump: addr=0x%08X, size=%6d bytes, pattern=0x%02x", 0x7FFE1234, 1024, 0xAA);

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

  std::string complex_msg =
      redlog::fmt("Complex: host=%s, pid=%05d, memory=%8.1fMB, flags=0x%04X", "server-01", 12345, 128.7, 0xABCD);
  log.info(complex_msg);

  // Performance comparison of new printf vs old
  std::cout << "\n--- Printf Performance Demonstration ---" << std::endl;
  const int test_iterations = 1000;

  auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < test_iterations; ++i) {
    std::string result = redlog::fmt("Iteration %04d: value=0x%08X, progress=%.2f%%", i, i * 0xABCD, (i / 10.0));
    // consume result to prevent optimization
    (void) result;
  }
  auto end = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  log.info_f(
      "Printf performance: %d iterations in %d microseconds (%.2f per call)", test_iterations, duration.count(),
      duration.count() / double(test_iterations)
  );
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
    log.info(
        "Complex message with fields", redlog::field("iteration", i),
        redlog::field(
            "timestamp",
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
                .count()
        ),
        redlog::field("cpu_usage", 45.7 + (i % 50)), redlog::field("memory_mb", 1024 + (i % 512))
    );
  }
  auto complex_time = std::chrono::steady_clock::now() - start;

  // Reset to info and show results
  redlog::set_level(redlog::level::info);
  std::cout << "--- Performance Results ---" << std::endl;

  log.info(
      "Filtering performance test", redlog::field("iterations", iterations),
      redlog::field("filtered_us", std::chrono::duration_cast<std::chrono::microseconds>(filtered_time).count()),
      redlog::field("enabled_us", std::chrono::duration_cast<std::chrono::microseconds>(enabled_time).count()),
      redlog::field(
          "speedup_factor",
          static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(enabled_time).count()) /
              std::max(
                  1.0, static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(filtered_time).count())
              )
      )
  );

  log.info(
      "Message complexity performance",
      redlog::field("simple_us", std::chrono::duration_cast<std::chrono::microseconds>(simple_time).count()),
      redlog::field("complex_us", std::chrono::duration_cast<std::chrono::microseconds>(complex_time).count()),
      redlog::field(
          "overhead_factor",
          static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(complex_time).count()) /
              static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(simple_time).count())
      )
  );
}

int main() {
  std::cout << "=== Themes and Formatting Demonstration ===" << std::endl;

  // Set initial level to show most messages
  redlog::set_level(redlog::level::debug);

  // Run basic demonstrations
  demonstrate_default_theme();
  demonstrate_plain_theme();
  demonstrate_custom_theme();

  // Run comprehensive custom demonstrations
  demonstrate_comprehensive_themes();
  demonstrate_comprehensive_formatters();
  demonstrate_advanced_custom_integration();

  // Run legacy formatter integration demo (now simplified)
  demonstrate_custom_formatters();

  // Technical demonstrations
  demonstrate_printf_formatting_comprehensive();
  demonstrate_environment_variables();
  demonstrate_performance_comparison();

  return 0;
}
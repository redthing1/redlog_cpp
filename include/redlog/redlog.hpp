#pragma once

#include <chrono>
#include <cstdio>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

// platform detection for TTY support
#ifdef _WIN32
    #include <io.h>
    #define REDLOG_IS_TTY(stream) _isatty(_fileno(stream))
#else
    #include <unistd.h>
    #define REDLOG_IS_TTY(stream) isatty(fileno(stream))
#endif

// compile-time log level filtering
#ifndef REDLOG_MIN_LEVEL
    #define REDLOG_MIN_LEVEL 3  // info level
#endif

namespace redlog {

/**
 * Log level enumeration with explicit ordering (lower values = higher priority).
 * 
 * Provides both full names and 3-character abbreviations for each level.
 * Default level is info (3).
 */
enum class level : int {
    critical = 0,   // crt - system-breaking errors
    error = 1,      // err - recoverable errors  
    warn = 2,       // wrn - warnings and potential issues
    info = 3,       // inf - general informational messages (default)
    verbose = 4,    // vrb - detailed operational information
    trace = 5,      // trc - detailed execution tracing
    debug = 6,      // dbg - debugging information
    pedantic = 7,   // ped - extremely detailed debugging
    annoying = 8    // ayg - maximum verbosity
};

// level utilities
constexpr std::string_view level_name(level l) noexcept {
    constexpr std::string_view names[] = {
        "critical", "error", "warn", "info", "verbose", 
        "trace", "debug", "pedantic", "annoying"
    };
    int idx = static_cast<int>(l);
    return (idx >= 0 && idx < 9) ? names[idx] : "unknown";
}

constexpr std::string_view level_short_name(level l) noexcept {
    constexpr std::string_view names[] = {
        "crt", "err", "wrn", "inf", "vrb", 
        "trc", "dbg", "ped", "ayg"
    };
    int idx = static_cast<int>(l);
    return (idx >= 0 && idx < 9) ? names[idx] : "unk";
}

// ansi color codes
enum class color : int {
    none = 0,
    red = 31, green = 32, yellow = 33, blue = 34, 
    magenta = 35, cyan = 36, white = 37,
    bright_red = 91, bright_green = 92, bright_yellow = 93,
    bright_blue = 94, bright_magenta = 95, bright_cyan = 96
};

/**
 * Theme configuration for visual appearance and layout.
 * 
 * Controls colors for different log levels and message components,
 * as well as formatting layout parameters.
 */
struct theme {
    // colors for each log level
    color critical_color = color::bright_magenta;
    color error_color = color::red;
    color warn_color = color::yellow;
    color info_color = color::green;
    color verbose_color = color::blue;
    color trace_color = color::white;
    color debug_color = color::bright_cyan;
    color pedantic_color = color::bright_cyan;
    color annoying_color = color::bright_cyan;
    
    // colors for message components
    color source_color = color::cyan;
    color message_color = color::white;
    color field_key_color = color::bright_cyan;
    color field_value_color = color::white;
    
    // layout configuration
    int source_width = 12;      // fixed width for source names
    int message_min_width = 35; // minimum width before fields start
};

namespace themes {
    inline constexpr theme default_theme{};
    
    // plain theme with no colors
    inline constexpr theme plain{
        .critical_color = color::none, .error_color = color::none,
        .warn_color = color::none, .info_color = color::none,
        .verbose_color = color::none, .trace_color = color::none,
        .debug_color = color::none, .pedantic_color = color::none,
        .annoying_color = color::none, .source_color = color::none,
        .message_color = color::none, .field_key_color = color::none,
        .field_value_color = color::none
    };
}

namespace detail {

// simple TTY and color detection
inline bool should_use_color() noexcept {
    static const bool use_color = []() -> bool {
        if (std::getenv("NO_COLOR") || std::getenv("REDLOG_NO_COLOR")) return false;
        if (std::getenv("FORCE_COLOR") || std::getenv("REDLOG_FORCE_COLOR")) return true;
        return REDLOG_IS_TTY(stderr);
    }();
    return use_color;
}

// simple ansi color formatting
inline std::string colorize(std::string_view text, color c) {
    if (!should_use_color() || c == color::none) {
        return std::string(text);
    }
    return "\033[" + std::to_string(static_cast<int>(c)) + "m" + std::string(text) + "\033[0m";
}

// SFINAE helpers for type detection
template<typename T, typename = void>
struct has_ostream_operator : std::false_type {};

template<typename T>
struct has_ostream_operator<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<T>())>> 
    : std::true_type {};

/**
 * Universal type-to-string conversion.
 * 
 * Tries multiple approaches in order:
 * 1. Direct string types
 * 2. Arithmetic types (using std::to_string)
 * 3. Types with operator<< (custom types can implement this)
 * 4. Fallback for unprintable types
 */
template<typename T>
std::string stringify(T&& value) {
    using decay_t = std::decay_t<T>;
    
    // handle string types directly
    if constexpr (std::is_same_v<decay_t, std::string>) {
        return value;
    } else if constexpr (std::is_convertible_v<decay_t, std::string_view>) {
        return std::string(value);
    } else if constexpr (std::is_same_v<decay_t, const char*> || std::is_same_v<decay_t, char*>) {
        return value ? std::string(value) : "null";
    }
    // handle arithmetic types
    else if constexpr (std::is_arithmetic_v<decay_t>) {
        return std::to_string(value);
    }
    // handle types with operator<<
    else if constexpr (has_ostream_operator<decay_t>::value) {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }
    // fallback for unprintable types
    else {
        return "[unprintable]";
    }
}

// global configuration
class config {
    level min_level_ = level::info;
    theme theme_ = themes::default_theme;
    mutable std::mutex mutex_;
    
public:
    static config& instance() {
        static config instance_;
        return instance_;
    }
    
    level min_level() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return min_level_;
    }
    
    void set_level(level l) {
        std::lock_guard<std::mutex> lock(mutex_);
        min_level_ = l;
    }
    
    theme get_theme() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return theme_;
    }
    
    void set_theme(const theme& t) {
        std::lock_guard<std::mutex> lock(mutex_);
        theme_ = t;
    }
};

} // namespace detail

/**
 * Simple structured field for key-value logging.
 * 
 * Just stores a key and value as strings - no complex lazy evaluation.
 */
struct field {
    std::string key;
    std::string value;
    
    template<typename T>
    field(std::string_view k, T&& v) 
        : key(k), value(detail::stringify(std::forward<T>(v))) {}
};

/**
 * Collection of fields with simple operations.
 * 
 * Optimized for the common case of small numbers of fields.
 */
class field_set {
    std::vector<field> fields_;
    
public:
    field_set() = default;
    field_set(std::initializer_list<field> fields) : fields_(fields) {}
    
    void add(field f) {
        fields_.emplace_back(std::move(f));
    }
    
    void merge(const field_set& other) {
        fields_.insert(fields_.end(), other.fields_.begin(), other.fields_.end());
    }
    
    // immutable operations for clean chaining
    field_set with_field(field f) const {
        field_set result = *this;
        result.add(std::move(f));
        return result;
    }
    
    field_set with_fields(const field_set& other) const {
        field_set result = *this;
        result.merge(other);
        return result;
    }
    
    const std::vector<field>& fields() const { return fields_; }
    bool empty() const { return fields_.empty(); }
    std::size_t size() const { return fields_.size(); }
};

/**
 * Log entry representing a single log message with metadata.
 */
struct log_entry {
    level level_val;
    std::string message;
    std::string source;
    field_set fields;
    std::chrono::system_clock::time_point timestamp;
    
    log_entry(level l, std::string msg, std::string src, field_set f)
        : level_val(l), message(std::move(msg)), source(std::move(src)), 
          fields(std::move(f)), timestamp(std::chrono::system_clock::now()) {}
};

/**
 * Formatter interface for customizable output.
 */
class formatter {
public:
    virtual ~formatter() = default;
    virtual std::string format(const log_entry& entry) const = 0;
};

/**
 * Default formatter producing beautiful aligned output.
 * 
 * Format: [source]      [lvl] message                    key=value key=value
 */
class default_formatter : public formatter {
    theme theme_;
    
    color level_color(level l) const {
        switch (l) {
        case level::critical: return theme_.critical_color;
        case level::error: return theme_.error_color;
        case level::warn: return theme_.warn_color;
        case level::info: return theme_.info_color;
        case level::verbose: return theme_.verbose_color;
        case level::trace: return theme_.trace_color;
        case level::debug: return theme_.debug_color;
        case level::pedantic: return theme_.pedantic_color;
        case level::annoying: return theme_.annoying_color;
        default: return color::white;
        }
    }
    
public:
    default_formatter() : theme_(detail::config::instance().get_theme()) {}
    explicit default_formatter(const theme& t) : theme_(t) {}
    
    std::string format(const log_entry& entry) const override {
        std::ostringstream oss;
        
        // source component with fixed width padding
        if (!entry.source.empty()) {
            std::string source_part = "[" + entry.source + "]";
            oss << detail::colorize(source_part, theme_.source_color);
            
            int padding = theme_.source_width - static_cast<int>(source_part.length());
            oss << std::string(std::max(1, padding), ' ');
        }
        
        // level component
        std::string level_part = "[" + std::string(level_short_name(entry.level_val)) + "]";
        oss << detail::colorize(level_part, level_color(entry.level_val)) << " ";
        
        // message component
        oss << detail::colorize(entry.message, theme_.message_color);
        
        // fields component with right alignment
        if (!entry.fields.empty()) {
            // calculate padding for field alignment
            int current_width = static_cast<int>(entry.source.length() + level_part.length() + entry.message.length()) + 5;
            int padding = std::max(4, theme_.message_min_width - current_width);
            oss << std::string(padding, ' ');
            
            // output fields
            bool first = true;
            for (const auto& f : entry.fields.fields()) {
                if (!first) oss << " ";
                first = false;
                
                oss << detail::colorize(f.key, theme_.field_key_color)
                    << "="
                    << detail::colorize(f.value, theme_.field_value_color);
            }
        }
        
        return oss.str();
    }
};

/**
 * Output sink interface.
 */
class sink {
public:
    virtual ~sink() = default;
    virtual void write(std::string_view formatted) = 0;
    virtual void flush() = 0;
};

/**
 * Console sink for stderr output.
 */
class console_sink : public sink {
public:
    void write(std::string_view formatted) override {
        std::fprintf(stderr, "%.*s\n", static_cast<int>(formatted.size()), formatted.data());
    }
    
    void flush() override {
        std::fflush(stderr);
    }
};

/**
 * File sink for logging to a file.
 * Falls back to stderr if file cannot be opened.
 */
class file_sink : public sink {
    FILE* file_;
    bool should_close_;
    
public:
    explicit file_sink(const std::string& filename) : file_(nullptr), should_close_(false) {
        file_ = std::fopen(filename.c_str(), "a");
        should_close_ = (file_ != nullptr);
        if (!file_) {
            // fallback to stderr if file open fails
            file_ = stderr;
            should_close_ = false;
        }
    }
    
    ~file_sink() {
        if (should_close_ && file_) {
            std::fclose(file_);
        }
    }
    
    // non-copyable, moveable
    file_sink(const file_sink&) = delete;
    file_sink& operator=(const file_sink&) = delete;
    file_sink(file_sink&&) = default;
    file_sink& operator=(file_sink&&) = default;
    
    void write(std::string_view formatted) override {
        if (file_) {
            std::fprintf(file_, "%.*s\n", static_cast<int>(formatted.size()), formatted.data());
        }
    }
    
    void flush() override {
        if (file_) {
            std::fflush(file_);
        }
    }
};

/**
 * Main logger class with immutable design for natural thread safety.
 * 
 * Loggers are immutable - methods like with_name() and with_field() return
 * new logger instances rather than modifying the original. This provides
 * clean scoping semantics and thread safety.
 */
class logger {
    std::string name_;
    field_set fields_;
    std::shared_ptr<formatter> formatter_;
    std::shared_ptr<sink> sink_;
    
public:
    /**
     * Create a logger with the given name.
     * Uses default console output and formatting.
     */
    explicit logger(std::string_view name = "")
        : name_(name),
          formatter_(std::make_shared<default_formatter>()),
          sink_(std::make_shared<console_sink>()) {}
    
    /**
     * Create a scoped logger with additional name component.
     * 
     * Example: logger("app").with_name("db") creates logger named "app.db"
     */
    logger with_name(std::string_view name) const {
        logger result = *this;
        if (result.name_.empty()) {
            result.name_ = name;
        } else {
            result.name_ += "." + std::string(name);
        }
        return result;
    }
    
    /**
     * Create a logger with additional field.
     * Field values are converted to strings using the universal stringify function.
     */
    template<typename T>
    logger with_field(std::string_view key, T&& value) const {
        logger result = *this;
        result.fields_.add(field(key, std::forward<T>(value)));
        return result;
    }
    
    /**
     * Create a logger with additional fields.
     */
    logger with_fields(const field_set& new_fields) const {
        logger result = *this;
        result.fields_.merge(new_fields);
        return result;
    }
    
    /**
     * Create a logger with multiple additional fields.
     */
    template<typename... Fields>
    logger with_fields(Fields&&... fields) const {
        logger result = *this;
        (result.fields_.add(std::forward<Fields>(fields)), ...);
        return result;
    }
    
private:
    // check if level should be logged
    bool should_log(level l) const {
        return static_cast<int>(l) <= REDLOG_MIN_LEVEL && 
               l <= detail::config::instance().min_level();
    }
    
    // core logging implementation
    void log_impl(level lvl, std::string_view msg) const {
        log_impl_with_fields(lvl, msg);
    }
    
    // core logging implementation with additional fields
    template<typename... Fields>
    void log_impl_with_fields(level lvl, std::string_view msg, Fields&&... fields) const {
        if (!should_log(lvl)) return;
        
        try {
            // create merged field set without copying the logger
            field_set entry_fields = fields_;
            (entry_fields.add(std::forward<Fields>(fields)), ...);
            
            log_entry entry(lvl, std::string(msg), name_, std::move(entry_fields));
            std::string formatted = formatter_->format(entry);
            sink_->write(formatted);
        } catch (...) {
            // fallback error handling
            std::fprintf(stderr, "[redlog-error] Failed to log: %.*s\n", 
                        static_cast<int>(msg.size()), msg.data());
        }
    }
    
public:
    // logging methods - full names
    
    template<typename... Fields>
    void critical(std::string_view msg, Fields&&... fields) const {
        log_impl_with_fields(level::critical, msg, std::forward<Fields>(fields)...);
    }
    
    template<typename... Fields>
    void error(std::string_view msg, Fields&&... fields) const {
        log_impl_with_fields(level::error, msg, std::forward<Fields>(fields)...);
    }
    
    template<typename... Fields>
    void warn(std::string_view msg, Fields&&... fields) const {
        log_impl_with_fields(level::warn, msg, std::forward<Fields>(fields)...);
    }
    
    template<typename... Fields>
    void info(std::string_view msg, Fields&&... fields) const {
        log_impl_with_fields(level::info, msg, std::forward<Fields>(fields)...);
    }
    
    template<typename... Fields>
    void verbose(std::string_view msg, Fields&&... fields) const {
        log_impl_with_fields(level::verbose, msg, std::forward<Fields>(fields)...);
    }
    
    template<typename... Fields>
    void trace(std::string_view msg, Fields&&... fields) const {
        log_impl_with_fields(level::trace, msg, std::forward<Fields>(fields)...);
    }
    
    template<typename... Fields>
    void debug(std::string_view msg, Fields&&... fields) const {
        log_impl_with_fields(level::debug, msg, std::forward<Fields>(fields)...);
    }
    
    template<typename... Fields>
    void pedantic(std::string_view msg, Fields&&... fields) const {
        log_impl_with_fields(level::pedantic, msg, std::forward<Fields>(fields)...);
    }
    
    template<typename... Fields>
    void annoying(std::string_view msg, Fields&&... fields) const {
        log_impl_with_fields(level::annoying, msg, std::forward<Fields>(fields)...);
    }
    
    // logging methods - short names
    template<typename... Fields> void crt(std::string_view msg, Fields&&... fields) const { critical(msg, std::forward<Fields>(fields)...); }
    template<typename... Fields> void err(std::string_view msg, Fields&&... fields) const { error(msg, std::forward<Fields>(fields)...); }
    template<typename... Fields> void wrn(std::string_view msg, Fields&&... fields) const { warn(msg, std::forward<Fields>(fields)...); }
    template<typename... Fields> void inf(std::string_view msg, Fields&&... fields) const { info(msg, std::forward<Fields>(fields)...); }
    template<typename... Fields> void vrb(std::string_view msg, Fields&&... fields) const { verbose(msg, std::forward<Fields>(fields)...); }
    template<typename... Fields> void trc(std::string_view msg, Fields&&... fields) const { trace(msg, std::forward<Fields>(fields)...); }
    template<typename... Fields> void dbg(std::string_view msg, Fields&&... fields) const { debug(msg, std::forward<Fields>(fields)...); }
    template<typename... Fields> void ped(std::string_view msg, Fields&&... fields) const { pedantic(msg, std::forward<Fields>(fields)...); }
    template<typename... Fields> void ayg(std::string_view msg, Fields&&... fields) const { annoying(msg, std::forward<Fields>(fields)...); }
    
    /**
     * Printf-style formatting with universal %s support.
     * 
     * All arguments are converted to strings using the universal stringify
     * function, then %s placeholders are replaced with the stringified values.
     * 
     * This means %s works with:
     * - Basic types (strings, numbers)
     * - Any custom type that implements operator<< for streams
     */
    template<typename... Args>
    void critical_f(const char* format, Args&&... args) const {
        if (!should_log(level::critical)) return;
        std::string msg = format_string(format, std::forward<Args>(args)...);
        critical(msg);
    }
    
    template<typename... Args>
    void error_f(const char* format, Args&&... args) const {
        if (!should_log(level::error)) return;
        std::string msg = format_string(format, std::forward<Args>(args)...);
        error(msg);
    }
    
    template<typename... Args>
    void warn_f(const char* format, Args&&... args) const {
        if (!should_log(level::warn)) return;
        std::string msg = format_string(format, std::forward<Args>(args)...);
        warn(msg);
    }
    
    template<typename... Args>
    void info_f(const char* format, Args&&... args) const {
        if (!should_log(level::info)) return;
        std::string msg = format_string(format, std::forward<Args>(args)...);
        info(msg);
    }
    
    template<typename... Args>
    void verbose_f(const char* format, Args&&... args) const {
        if (!should_log(level::verbose)) return;
        std::string msg = format_string(format, std::forward<Args>(args)...);
        verbose(msg);
    }
    
    template<typename... Args>
    void trace_f(const char* format, Args&&... args) const {
        if (!should_log(level::trace)) return;
        std::string msg = format_string(format, std::forward<Args>(args)...);
        trace(msg);
    }
    
    template<typename... Args>
    void debug_f(const char* format, Args&&... args) const {
        if (!should_log(level::debug)) return;
        std::string msg = format_string(format, std::forward<Args>(args)...);
        debug(msg);
    }
    
    template<typename... Args>
    void pedantic_f(const char* format, Args&&... args) const {
        if (!should_log(level::pedantic)) return;
        std::string msg = format_string(format, std::forward<Args>(args)...);
        pedantic(msg);
    }
    
    template<typename... Args>
    void annoying_f(const char* format, Args&&... args) const {
        if (!should_log(level::annoying)) return;
        std::string msg = format_string(format, std::forward<Args>(args)...);
        annoying(msg);
    }
    
    // short form printf methods
    template<typename... Args> void crt_f(const char* format, Args&&... args) const { critical_f(format, std::forward<Args>(args)...); }
    template<typename... Args> void err_f(const char* format, Args&&... args) const { error_f(format, std::forward<Args>(args)...); }
    template<typename... Args> void wrn_f(const char* format, Args&&... args) const { warn_f(format, std::forward<Args>(args)...); }
    template<typename... Args> void inf_f(const char* format, Args&&... args) const { info_f(format, std::forward<Args>(args)...); }
    template<typename... Args> void vrb_f(const char* format, Args&&... args) const { verbose_f(format, std::forward<Args>(args)...); }
    template<typename... Args> void trc_f(const char* format, Args&&... args) const { trace_f(format, std::forward<Args>(args)...); }
    template<typename... Args> void dbg_f(const char* format, Args&&... args) const { debug_f(format, std::forward<Args>(args)...); }
    template<typename... Args> void ped_f(const char* format, Args&&... args) const { pedantic_f(format, std::forward<Args>(args)...); }
    template<typename... Args> void ayg_f(const char* format, Args&&... args) const { annoying_f(format, std::forward<Args>(args)...); }
    
private:
    /**
     * Simple printf-style string formatting.
     * 
     * Converts all arguments to strings, then replaces %s placeholders
     * in order. This gives us universal %s support for any type that
     * can be stringified.
     */
    template<typename... Args>
    std::string format_string(const char* format, Args&&... args) const {
        if constexpr (sizeof...(args) == 0) {
            return std::string(format);
        } else {
            // convert all arguments to strings
            std::vector<std::string> string_args = {
                detail::stringify(std::forward<Args>(args))...
            };
            
            std::string result(format);
            std::size_t arg_index = 0;
            std::size_t pos = 0;
            
            // replace %s with stringified arguments
            while ((pos = result.find("%s", pos)) != std::string::npos && arg_index < string_args.size()) {
                result.replace(pos, 2, string_args[arg_index]);
                pos += string_args[arg_index].length();
                ++arg_index;
            }
            
            return result;
        }
    }
};

// global configuration functions

/**
 * Set the global minimum log level.
 * Messages below this level will be filtered out.
 */
inline void set_level(level l) {
    detail::config::instance().set_level(l);
}

/**
 * Get the current global minimum log level.
 */
inline level get_level() {
    return detail::config::instance().min_level();
}

/**
 * Set the global theme for colors and formatting.
 */
inline void set_theme(const theme& t) {
    detail::config::instance().set_theme(t);
}

/**
 * Get the current global theme.
 */
inline theme get_theme() {
    return detail::config::instance().get_theme();
}

/**
 * Create a logger with the given name.
 * 
 * This is the main entry point for creating loggers.
 * Example: auto log = redlog::get_logger("app");
 */
inline logger get_logger(std::string_view name = "") {
    return logger(name);
}

} // namespace redlog

// cleanup
#undef REDLOG_IS_TTY
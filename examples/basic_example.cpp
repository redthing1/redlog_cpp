#include <redlog/redlog.hpp>
#include <thread>
#include <chrono>

int main() {
    using namespace redlog;
    
    // Demonstrate different log levels and verbosity
    auto log = get_logger("example");
    
    // Set to debug level to see all messages
    set_level(level::debug);
    
    log.critical("Critical system error - immediate attention required");
    log.error("Error occurred during processing");
    log.warn("Warning: deprecated API usage detected");
    log.info("Application started successfully");
    log.verbose("Verbose: detailed operational information");
    log.trace("Trace: function entry/exit tracking");
    log.debug("Debug: variable values and state information");
    log.pedantic("Pedantic: extremely detailed debugging info");
    log.annoying("Annoying: maximum verbosity level");
    
    // Short form methods for all levels
    log.crt("Critical using short form");
    log.err("Error using short form");
    log.wrn("Warning using short form");
    log.inf("Info using short form");
    log.vrb("Verbose using short form");
    log.trc("Trace using short form");
    log.dbg("Debug using short form");
    log.ped("Pedantic using short form");
    log.ayg("Annoying using short form");
    
    // Logging with fields
    log.info("User login attempt", 
             field("username", "alice"),
             field("ip_address", "192.168.1.100"),
             field("success", true));
    
    // Different data types
    log.info("Data types example",
             field("string", "hello world"),
             field("integer", 42),
             field("float", 3.14159),
             field("boolean", false));
    
    // Scoped logger with hierarchical naming
    auto db_log = log.with_name("database");
    db_log.info("Database connection established");
    
    auto query_log = db_log.with_name("query");
    query_log.debug("Executing SELECT statement", field("table", "users"));
    
    // Logger with persistent fields
    auto request_log = log.with_field("request_id", 12345)
                          .with_field("method", "GET");
    
    request_log.info("Request started", field("path", "/api/users"));
    request_log.info("Request completed", field("status", 200), field("duration_ms", 150));
    
    // Printf-style formatting with proper format specifiers
    log.info_f("Server listening on port %d", 8080);
    log.error_f("Failed to connect to %s:%d", "database.example.com", 5432);
    log.debug_f("Hex value: 0x%x, Float: %.2f", 255, 3.14159);
    log.verbose_f("Octal: %o, Scientific: %e", 64, 1234.5);
    log.trace_f("Character: %c, Uppercase hex: %X", 65, 255);
    
    // General formatting function - useful for non-logging string formatting
    std::string formatted_msg = fmt("Processing %d items with %.1f%% efficiency", 42, 95.7);
    log.info(formatted_msg);
    
    // Demonstrate level filtering - this will filter out lower priority messages
    log.info("\n=== Changing log level to INFO ===");
    set_level(level::info);  // This will disable verbose, trace, debug, pedantic, annoying
    
    log.critical("Critical still visible");
    log.error("Error still visible");
    log.warn("Warning still visible");
    log.info("Info still visible");
    log.verbose("Verbose - should not appear");
    log.debug("Debug - should not appear");
    log.pedantic("Pedantic - should not appear");
    
    // Demonstrate even more restrictive filtering
    log.info("\n=== Changing log level to ERROR ===");
    set_level(level::error);
    
    log.critical("Critical still visible");
    log.error("Error still visible");
    log.warn("Warning - should not appear");
    log.info("Info - should not appear");
    
    // Reset to info for performance demonstration
    set_level(level::info);
    
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 10000; ++i) {
        log.debug("This debug message is filtered out", field("iteration", i));
    }
    auto end = std::chrono::steady_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    log.info("Performance test completed", 
             field("iterations", 10000),
             field("total_time_us", duration.count()),
             field("avg_time_ns", duration.count() * 1000 / 10000));
    
    // Demonstrate all printf levels with different verbosity
    log.info("\n=== Printf-style formatting at different levels ===");
    set_level(level::annoying);  // Show all levels
    
    log.critical_f("Critical: System has %d critical errors", 3);
    log.error_f("Error: Failed to process %d/%d items", 5, 100);
    log.warn_f("Warning: Memory usage at %.1f%% capacity", 85.7);
    log.info_f("Info: Processing batch %d of %d", 7, 10);
    log.verbose_f("Verbose: Thread pool has %d active workers", 8);
    log.trace_f("Trace: Function entry with parameter value %x", 0xDEADBEEF);
    log.debug_f("Debug: Variable state - counter=%d, flag=%c", 42, 'Y');
    log.pedantic_f("Pedantic: Detailed timing - %.3f seconds elapsed", 1.234567);
    log.annoying_f("Annoying: Buffer state - %o octal representation", 755);
    
    return 0;
}
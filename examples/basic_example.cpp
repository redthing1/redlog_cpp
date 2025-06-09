#include <redlog/redlog.hpp>
#include <thread>
#include <chrono>

int main() {
    using namespace redlog;
    
    // Basic logging with different levels
    auto log = get_logger("example");
    
    log.info("Application started");
    log.debug("Debug message - might not be visible depending on level");
    log.warn("This is a warning");
    log.error("This is an error message");
    
    // Short form methods
    log.inf("Info using short form");
    log.dbg("Debug using short form");
    log.wrn("Warning using short form");
    log.err("Error using short form");
    
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
    
    // Printf-style formatting (universal %s support)
    log.info_f("Server listening on port %s", 8080);
    log.error_f("Failed to connect to %s:%s", "database.example.com", 5432);
    
    // Performance demonstration - fast path for disabled levels
    set_level(level::info);  // This will disable debug messages
    
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
    
    return 0;
}
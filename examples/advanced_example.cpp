#include <redlog/redlog.hpp>
#include <thread>
#include <vector>
#include <chrono>
#include <stdexcept>

// Custom type with stream operator
struct custom_object {
    int id;
    std::string name;
    
    friend std::ostream& operator<<(std::ostream& os, const custom_object& obj) {
        return os << "CustomObject{id=" << obj.id << ", name=" << obj.name << "}";
    }
};

// Simulated database class showing practical usage patterns
class database_manager {
    redlog::logger log_;
    
public:
    database_manager() : log_(redlog::get_logger("db")) {
        log_.info("Database manager initialized");
    }
    
    void connect(const std::string& host, int port) {
        auto conn_log = log_.with_field("host", host)
                           .with_field("port", port);
        
        conn_log.info("Attempting connection");
        
        // Simulate connection logic with potential failure
        if (host == "bad-host") {
            conn_log.error("Connection failed", redlog::field("reason", "host unreachable"));
            throw std::runtime_error("Connection failed");
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        conn_log.info("Connected successfully");
    }
    
    void execute_query(const std::string& sql) {
        static int query_id = 0;
        auto query_log = log_.with_name("query")
                            .with_field("query_id", ++query_id);
        
        query_log.trace("Executing query", redlog::field("sql", sql));
        
        auto start = std::chrono::steady_clock::now();
        
        // Simulate query execution
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        auto duration = std::chrono::steady_clock::now() - start;
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        
        query_log.debug_f("Query completed in %d ms, affected %d rows", 
                          static_cast<int>(duration_ms), 42);
        query_log.trace("Query performance",
                       redlog::field("duration_ms", duration_ms),
                       redlog::field("rows_affected", 42));
    }
};

// HTTP request simulation showing context propagation
struct http_request {
    std::string method;
    std::string path;
    std::string client_ip;
    int request_id;
    
    http_request(std::string m, std::string p, std::string ip, int id)
        : method(std::move(m)), path(std::move(p)), client_ip(std::move(ip)), request_id(id) {}
};

void handle_http_request(const http_request& req) {
    auto request_log = redlog::get_logger("http")
                          .with_field("method", req.method)
                          .with_field("path", req.path)
                          .with_field("request_id", req.request_id)
                          .with_field("client_ip", req.client_ip);
    
    request_log.info("Request started");
    
    try {
        // Simulate request processing
        if (req.path == "/api/users") {
            auto db_log = request_log.with_name("db");
            database_manager db;
            db.connect("localhost", 5432);
            db.execute_query("SELECT * FROM users");
            
            request_log.info_f("Request completed: %d status, %d bytes", 200, 1024);
            request_log.debug("Response details",
                            redlog::field("status_code", 200),
                            redlog::field("response_size", 1024));
            
        } else if (req.path == "/api/error") {
            throw std::runtime_error("Simulated error");
            
        } else {
            request_log.warn_f("Unknown endpoint: %s (status %d)", req.path.c_str(), 404);
        }
        
    } catch (const std::exception& e) {
        request_log.error_f("Request failed with status %d: %s", 500, e.what());
        request_log.debug("Error details",
                         redlog::field("error", e.what()),
                         redlog::field("status_code", 500));
    }
}

// Thread safety demonstration
void worker_thread(int thread_id) {
    auto log = redlog::get_logger("worker")
                  .with_field("thread_id", thread_id);
    
    // Different threads demonstrate different verbosity levels
    switch (thread_id % 4) {
        case 0:
            log.info("Worker thread started (using INFO level)");
            break;
        case 1:
            log.verbose("Worker thread started (using VERBOSE level)");
            break;
        case 2:
            log.debug("Worker thread started (using DEBUG level)");
            break;
        case 3:
            log.trace("Worker thread started (using TRACE level)");
            break;
    }
    
    for (int i = 0; i < 5; ++i) {
        switch (thread_id % 4) {
            case 0:
                log.info("Processing item", redlog::field("item", i));
                break;
            case 1:
                log.verbose("Processing item with verbose details", redlog::field("item", i), redlog::field("memory_mb", 128 + i * 10));
                break;
            case 2:
                log.debug("Processing item with debug info", redlog::field("item", i), redlog::field("cpu_percent", 15.5 + i * 2.1));
                break;
            case 3:
                log.trace("Processing item with trace details", redlog::field("item", i), redlog::field("timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()));
                break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    switch (thread_id % 4) {
        case 0:
            log.info("Worker thread completed");
            break;
        case 1:
            log.verbose("Worker thread completed with all items processed");
            break;
        case 2:
            log.debug("Worker thread completed - releasing resources");
            break;
        case 3:
            log.trace("Worker thread completed - execution trace finished");
            break;
    }
}

int main() {
    using namespace redlog;
    
    std::cout << "=== redlog Advanced Example ===" << std::endl;
    std::cout << "Demonstrating complex scenarios, threading, performance, and real-world patterns" << std::endl;
    
    auto log = get_logger("advanced");
    
    // Start with info level to show basic scenarios
    std::cout << "\n--- Starting with INFO level ---" << std::endl;
    std::cout << "Current level: " << level_name(get_level()) << " (" << static_cast<int>(get_level()) << ")" << std::endl;
    
    log.info("Advanced example started - demonstrating complex patterns");
    
    // Custom type logging
    custom_object obj{123, "test_object"};
    log.info("Custom object logging", redlog::field("object", obj));
    
    // Using the general fmt function for string formatting
    std::string status_msg = fmt("System ready: %d cores, %dMB RAM, %.1f%% disk free", 8, 16384, 67.3);
    log.info("System status", redlog::field("status", status_msg));
    
    // Enable debug level to show detailed internal operations
    std::cout << "\n--- Enabling DEBUG level for detailed logging ---" << std::endl;
    set_level(level::debug);
    std::cout << "Current level: " << level_name(get_level()) << " (" << static_cast<int>(get_level()) << ")" << std::endl;
    
    // Now show all relevant log levels with realistic scenarios
    log.critical("System overload detected - immediate intervention required");
    log.error("Database connection lost - attempting reconnection");
    log.warn("High memory usage detected - consider scaling");
    log.info("User session established");
    log.verbose("Detailed request processing information");
    log.trace("Function call trace: process_request() entered");
    log.debug("Variable state: connection_count=42, active_sessions=15");
    
    // Printf-style formatting demonstrations  
    log.info_f("Server stats: %d connections, %.1f%% CPU usage", 42, 85.7);
    log.debug_f("Memory address: %p, hex value: 0x%x", &obj, 0xDEADBEEF);
    log.verbose_f("Process ID: %d, thread count: %d", 1234, 8);
    log.trace_f("Precision test: %.0f, %.2f, %.5f", 3.14159, 3.14159, 3.14159);
    
    // Enable maximum verbosity to show extreme debugging levels
    std::cout << "\n--- Enabling ANNOYING level (maximum verbosity) ---" << std::endl;
    set_level(level::annoying);
    std::cout << "Current level: " << level_name(get_level()) << " (shows everything)" << std::endl;
    
    log.pedantic("Memory allocation details: 1024 bytes allocated at 0x7fff");
    log.annoying("Micro-optimization: loop iteration 573 of 10000");
    
    // Demonstrate level filtering effects
    std::cout << "\n--- Demonstrating level filtering effects ---" << std::endl;
    std::cout << "Setting to WARN level (restrictive)" << std::endl;
    set_level(level::warn);
    
    log.critical("Critical: Still visible at WARN level");
    log.error("Error: Still visible at WARN level");
    log.warn("Warning: Still visible at WARN level");
    log.info("Info: Should not appear at WARN level");
    log.verbose("Verbose: Should not appear at WARN level");
    log.debug("Debug: Should not appear at WARN level");
    
    // Reset to verbose for detailed demo
    std::cout << "\n--- Setting to VERBOSE level for detailed operations ---" << std::endl;
    set_level(level::verbose);
    std::cout << "Current level: " << level_name(get_level()) << " (" << static_cast<int>(get_level()) << ")" << std::endl;
    
    // Theme configuration
    std::cout << "\n--- Theme configuration ---" << std::endl;
    log.info("Testing default theme with colors");
    
    // Switch to plain theme for CI environments
    if (std::getenv("CI")) {
        set_theme(themes::plain);
        log.info("Switched to plain theme for CI environment");
    }
    
    // HTTP request simulation
    std::cout << "\n--- HTTP request simulation with scoped logging ---" << std::endl;
    std::vector<http_request> requests = {
        {"GET", "/api/users", "192.168.1.100", 1001},
        {"POST", "/api/users", "192.168.1.101", 1002},
        {"GET", "/api/error", "192.168.1.102", 1003},
        {"GET", "/unknown", "192.168.1.103", 1004}
    };
    
    for (const auto& req : requests) {
        handle_http_request(req);
    }
    
    // Thread safety demonstration with different verbosity per thread
    std::cout << "\n--- Thread safety demonstration ---" << std::endl;
    log.verbose("Starting multi-threaded logging with different verbosity levels");
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back(worker_thread, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Performance comparison at different verbosity levels
    std::cout << "\n--- Performance impact of verbosity levels ---" << std::endl;
    log.verbose("Testing performance impact of level filtering");
    
    const int iterations = 1000;  // Smaller number for demo purposes
    
    // Test with very restrictive filtering (only critical/error)
    std::cout << "Testing with ERROR level (most restrictive)" << std::endl;
    set_level(level::error);
    auto start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        if (i < 5) {
            log.debug("Filtered debug message", field("iteration", i));
            log.verbose("Filtered verbose message", field("iteration", i));
            log.info("Filtered info message", field("iteration", i));
        }
    }
    
    auto restrictive_time = std::chrono::steady_clock::now() - start;
    
    // Test with moderate filtering (info and above)
    std::cout << "Testing with INFO level (moderate filtering)" << std::endl;
    set_level(level::info);
    start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        if (i < 5) {
            log.debug("Filtered debug message", field("iteration", i));
            log.verbose("Filtered verbose message", field("iteration", i));
            log.info("Enabled info message", field("iteration", i));
        }
    }
    
    auto moderate_time = std::chrono::steady_clock::now() - start;
    
    // Test with maximum verbosity (all messages enabled)
    std::cout << "Testing with ANNOYING level (maximum verbosity)" << std::endl;
    set_level(level::annoying);
    start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        if (i < 5) {
            log.debug("Enabled debug message", field("iteration", i));
            log.verbose("Enabled verbose message", field("iteration", i));
            log.info("Enabled info message", field("iteration", i));
        }
    }
    
    auto verbose_time = std::chrono::steady_clock::now() - start;
    
    // Reset to info level for final summary
    std::cout << "\n--- Performance results ---" << std::endl;
    set_level(level::info);
    std::cout << "Current level: " << level_name(get_level()) << " (for results display)" << std::endl;
    
    log.info("Performance comparison completed",
             redlog::field("iterations", iterations),
             redlog::field("restrictive_time_us", std::chrono::duration_cast<std::chrono::microseconds>(restrictive_time).count()),
             redlog::field("moderate_time_us", std::chrono::duration_cast<std::chrono::microseconds>(moderate_time).count()),
             redlog::field("verbose_time_us", std::chrono::duration_cast<std::chrono::microseconds>(verbose_time).count()));
    
    log.info("Performance analysis shows significant speedup with level filtering");
    
    std::cout << "\n=== Advanced example completed! ===" << std::endl;
    std::cout << "Key takeaways:" << std::endl;
    std::cout << "- Level filtering provides major performance benefits" << std::endl;
    std::cout << "- Scoped loggers enable contextual logging" << std::endl;
    std::cout << "- Thread-safe by design for concurrent applications" << std::endl;
    
    return 0;
}
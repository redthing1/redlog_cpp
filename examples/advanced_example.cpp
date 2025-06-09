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
    
    log.info("Worker thread started");
    
    for (int i = 0; i < 5; ++i) {
        log.debug("Processing item", redlog::field("item", i));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    log.info("Worker thread completed");
}

int main() {
    using namespace redlog;
    
    std::cout << "=== Advanced redlog Features Demo ===\n\n";
    
    // Set debug level to see all messages
    set_level(level::debug);
    
    auto log = get_logger("advanced");
    log.info("Advanced example started");
    
    // Custom type logging
    custom_object obj{123, "test_object"};
    log.info("Custom object logging", redlog::field("object", obj));
    
    // Printf-style formatting demonstrations
    log.info_f("Server stats: %d connections, %.1f%% CPU usage", 42, 85.7);
    log.debug_f("Memory address: %p, hex value: 0x%x", &obj, 0xDEADBEEF);
    log.verbose_f("Process ID: %d, thread count: %d", 1234, 8);
    log.trace_f("Precision test: %.0f, %.2f, %.5f", 3.14159, 3.14159, 3.14159);
    
    // Using the general fmt function for string formatting
    std::string status_msg = fmt("System ready: %d cores, %dMB RAM, %.1f%% disk free", 8, 16384, 67.3);
    log.info("System status", redlog::field("status", status_msg));
    
    // Theme configuration
    log.info("Testing default theme");
    
    // Switch to plain theme for CI environments
    if (std::getenv("CI")) {
        set_theme(themes::plain);
        log.info("Switched to plain theme for CI");
    }
    
    // HTTP request simulation
    std::vector<http_request> requests = {
        {"GET", "/api/users", "192.168.1.100", 1001},
        {"POST", "/api/users", "192.168.1.101", 1002},
        {"GET", "/api/error", "192.168.1.102", 1003},
        {"GET", "/unknown", "192.168.1.103", 1004}
    };
    
    for (const auto& req : requests) {
        handle_http_request(req);
    }
    
    // Thread safety demonstration
    log.info("Starting thread safety demonstration");
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back(worker_thread, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Performance comparison
    log.info("Performance comparison starting");
    
    const int iterations = 100000;
    
    // Test with level filtering
    set_level(level::warn);  // Disable info and debug
    auto start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        log.debug("Filtered message", field("iteration", i));
    }
    
    auto filtered_time = std::chrono::steady_clock::now() - start;
    
    // Test with level enabled
    set_level(level::debug);
    start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        log.debug("Enabled message", field("iteration", i));
    }
    
    auto enabled_time = std::chrono::steady_clock::now() - start;
    
    // Reset to info level for final summary
    set_level(level::info);
    
    log.info("Performance comparison completed",
             redlog::field("iterations", iterations),
             redlog::field("filtered_time_us", std::chrono::duration_cast<std::chrono::microseconds>(filtered_time).count()),
             redlog::field("enabled_time_us", std::chrono::duration_cast<std::chrono::microseconds>(enabled_time).count()));
    
    log.info("Advanced example completed");
    
    return 0;
}
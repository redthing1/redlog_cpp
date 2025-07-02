#include <chrono>
#include <iostream>
#include <redlog.hpp>
#include <thread>

int main() {
  using namespace redlog;

  // Create our main logger
  auto log = get_logger("example");

  std::cout << "=== redlog Basic Example ===" << std::endl;
  std::cout << "Demonstrating different log levels, verbosity filtering, and features" << std::endl;

  // Start with default level (info) to show basic usage
  std::cout << "\n--- Basic logging at INFO level (default) ---" << std::endl;
  std::cout << "Current level: " << level_name(get_level()) << " (" << static_cast<int>(get_level()) << ")"
            << std::endl;

  log.critical("Critical system error - immediate attention required");
  log.error("Error occurred during processing");
  log.warn("Warning: deprecated API usage detected");
  log.info("Application started successfully");
  log.verbose("Verbose - will not appear (filtered out)");
  log.debug("Debug - will not appear (filtered out)");

  // Demonstrate structured logging with fields
  std::cout << "\n--- Structured logging with fields ---" << std::endl;
  log.info(
      "User login attempt", field("username", "alice"), field("ip_address", "192.168.1.100"), field("success", true)
  );

  log.info(
      "Data types example", field("string", "hello world"), field("integer", 42), field("float", 3.14159),
      field("boolean", false)
  );

  // Demonstrate scoped loggers
  std::cout << "\n--- Scoped loggers ---" << std::endl;
  auto db_log = log.with_name("database");
  db_log.info("Database connection established");

  auto request_log = log.with_field("request_id", 12345).with_field("method", "GET");
  request_log.info("Request started", field("path", "/api/users"));
  request_log.info("Request completed", field("status", 200), field("duration_ms", 150));

  // Demonstrate printf-style formatting
  std::cout << "\n--- Printf-style formatting ---" << std::endl;
  log.info_f("Server listening on port %d", 8080);
  log.error_f("Failed to connect to %s:%d", "database.example.com", 5432);
  log.info_f("Processing %d items with %.1f%% efficiency", 42, 95.7);

  // Now demonstrate enabling debug level to see more messages
  std::cout << "\n--- Enabling DEBUG level (shows verbose, trace, debug) ---" << std::endl;
  set_level(level::debug);
  std::cout << "Current level: " << level_name(get_level()) << " (" << static_cast<int>(get_level()) << ")"
            << std::endl;

  log.critical("Critical still visible");
  log.error("Error still visible");
  log.warn("Warning still visible");
  log.info("Info still visible");
  log.verbose("Verbose now visible!");
  log.trace("Trace now visible!");
  log.debug("Debug now visible!");
  log.pedantic("Pedantic - still filtered (level 7 > 6)");

  // Show short form methods
  std::cout << "\n--- Short form methods ---" << std::endl;
  log.crt("Critical using short form");
  log.err("Error using short form");
  log.inf("Info using short form");
  log.dbg("Debug using short form");

  // Demonstrate more restrictive filtering
  std::cout << "\n--- Setting to WARN level (only critical, error, warn) ---" << std::endl;
  set_level(level::warn);
  std::cout << "Current level: " << level_name(get_level()) << " (" << static_cast<int>(get_level()) << ")"
            << std::endl;

  log.critical("Critical still visible");
  log.error("Error still visible");
  log.warn("Warning still visible");
  log.info("Info - now filtered out");
  log.debug("Debug - now filtered out");

  // Demonstrate performance of filtered messages
  std::cout << "\n--- Performance test with filtered messages ---" << std::endl;
  std::cout << "Testing 10,000 debug calls (should be very fast since they're filtered)" << std::endl;

  auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < 10000; ++i) {
    log.debug("This debug message is filtered out", field("iteration", i));
  }
  auto end = std::chrono::steady_clock::now();

  // Reset to info to show performance results
  set_level(level::info);
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  log.info(
      "Performance test completed", field("iterations", 10000), field("total_time_us", duration.count()),
      field("avg_time_ns", duration.count() * 1000 / 10000)
  );

  // Show all levels with printf formatting
  std::cout << "\n--- All log levels with printf formatting ---" << std::endl;
  set_level(level::annoying); // Enable all levels
  std::cout << "Current level: " << level_name(get_level()) << " (shows all levels)" << std::endl;

  log.critical_f("Critical: System has %d critical errors", 3);
  log.error_f("Error: Failed to process %d/%d items", 5, 100);
  log.warn_f("Warning: Memory usage at %.1f%% capacity", 85.7);
  log.info_f("Info: Processing batch %d of %d", 7, 10);
  log.verbose_f("Verbose: Thread pool has %d active workers", 8);
  log.trace_f("Trace: Function entry with parameter 0x%x", 0xDEADBEEF);
  log.debug_f("Debug: Variable state - counter=%d, flag=%c", 42, 'Y');
  log.pedantic_f("Pedantic: Detailed timing - %.3f seconds elapsed", 1.234567);
  log.annoying_f("Annoying: Buffer state - %o octal representation", 755);

  std::cout << "\n=== Example completed! ===" << std::endl;
  std::cout << "Try setting REDLOG_NO_COLOR=1 to disable colors" << std::endl;

  return 0;
}
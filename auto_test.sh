#!/bin/bash

# CMPE476 Distributed Systems - Automated Testing Script
# This script performs comprehensive testing of the distributed system

echo "=============================================="
echo "CMPE476 Distributed Systems - Automated Tests"
echo "=============================================="

# Global variables
TEST_LOG="test_results_$(date +%Y%m%d_%H%M%S).log"
PASS_COUNT=0
FAIL_COUNT=0
WATCHDOG_PID=""

# Logging function
log_test() {
    local test_name="$1"
    local result="$2"
    local details="$3"
    
    if [ "$result" = "PASS" ]; then
        ((PASS_COUNT++))
        echo "✅ $test_name: PASS" | tee -a "$TEST_LOG"
    else
        ((FAIL_COUNT++))
        echo "❌ $test_name: FAIL - $details" | tee -a "$TEST_LOG"
    fi
}

# Cleanup function
cleanup() {
    echo "Cleaning up processes and files..."
    
    # Kill watchdog if running
    if [ -n "$WATCHDOG_PID" ] && kill -0 "$WATCHDOG_PID" 2>/dev/null; then
        kill -SIGTSTP "$WATCHDOG_PID" 2>/dev/null
        sleep 2
    fi
    
    # Force kill any remaining processes
    pkill -f watchdog 2>/dev/null
    pkill -f load_balancer 2>/dev/null
    pkill -f reverse_proxy 2>/dev/null
    pkill -f server 2>/dev/null
    
    # Clean socket files
    rm -f /tmp/server_* /tmp/reverse_proxy_* /tmp/load_balancer 2>/dev/null
    
    sleep 1
}

# Trap for cleanup on exit
trap cleanup EXIT

echo "Starting automated testing at $(date)" | tee "$TEST_LOG"
echo "Test log: $TEST_LOG" | tee -a "$TEST_LOG"
echo ""

# Phase 1: Compilation Testing
echo "Phase 1: Compilation Testing" | tee -a "$TEST_LOG"
echo "==============================" | tee -a "$TEST_LOG"

make clean >/dev/null 2>&1
if make >/dev/null 2>&1; then
    if [ -x "./watchdog" ] && [ -x "./load_balancer" ] && [ -x "./reverse_proxy" ] && [ -x "./server" ] && [ -x "./client" ]; then
        log_test "Compilation" "PASS"
    else
        log_test "Compilation" "FAIL" "Some executables missing or not executable"
        exit 1
    fi
else
    log_test "Compilation" "FAIL" "Make command failed"
    exit 1
fi

# Phase 2: Basic System Startup
echo ""
echo "Phase 2: System Startup Testing" | tee -a "$TEST_LOG"
echo "================================" | tee -a "$TEST_LOG"

# Start watchdog in background
./watchdog > system.log 2>&1 &
WATCHDOG_PID=$!
sleep 3

# Check if watchdog is running
if kill -0 "$WATCHDOG_PID" 2>/dev/null; then
    log_test "Watchdog Startup" "PASS"
else
    log_test "Watchdog Startup" "FAIL" "Watchdog process died"
    exit 1
fi

# Check if all processes are running
sleep 2
PROCESS_COUNT=$(ps -ef | grep -E "(load_balancer|reverse_proxy|server)" | grep -v grep | wc -l)
if [ "$PROCESS_COUNT" -eq 9 ]; then
    log_test "All Processes Started" "PASS"
else
    log_test "All Processes Started" "FAIL" "Expected 9 processes, found $PROCESS_COUNT"
fi

# Phase 3: Load Balancing Testing
echo ""
echo "Phase 3: Load Balancing Testing" | tee -a "$TEST_LOG"
echo "===============================" | tee -a "$TEST_LOG"

# Test odd client (should go to proxy 1)
echo "16" | timeout 5 ./client 107 > client_107.out 2>&1
if grep -q "4.0" client_107.out; then
    log_test "Odd Client Routing" "PASS"
else
    log_test "Odd Client Routing" "FAIL" "Client 107 did not receive expected result"
fi

# Test even client (should go to proxy 2)
echo "9" | timeout 5 ./client 112 > client_112.out 2>&1
if grep -q "3.0" client_112.out; then
    log_test "Even Client Routing" "PASS"
else
    log_test "Even Client Routing" "FAIL" "Client 112 did not receive expected result"
fi

# Phase 4: Input Validation Testing
echo ""
echo "Phase 4: Input Validation Testing" | tee -a "$TEST_LOG"
echo "==================================" | tee -a "$TEST_LOG"

# Test negative input
echo "-4" | timeout 5 ./client 101 > client_neg.out 2>&1
if grep -q "\-1.0" client_neg.out; then
    log_test "Negative Input Validation" "PASS"
else
    log_test "Negative Input Validation" "FAIL" "Negative input not properly handled"
fi

# Phase 5: Concurrent Client Testing
echo ""
echo "Phase 5: Concurrent Client Testing" | tee -a "$TEST_LOG"
echo "===================================" | tee -a "$TEST_LOG"

# Start multiple concurrent clients
for i in {121..125}; do
    echo "25" | timeout 5 ./client $i > "client_$i.out" 2>&1 &
done
wait

# Check results
SUCCESS_COUNT=0
for i in {121..125}; do
    if grep -q "5.0" "client_$i.out"; then
        ((SUCCESS_COUNT++))
    fi
done

if [ "$SUCCESS_COUNT" -eq 5 ]; then
    log_test "Concurrent Clients" "PASS"
else
    log_test "Concurrent Clients" "FAIL" "Only $SUCCESS_COUNT out of 5 clients succeeded"
fi

# Phase 6: Process Recovery Testing
echo ""
echo "Phase 6: Process Recovery Testing" | tee -a "$TEST_LOG"
echo "==================================" | tee -a "$TEST_LOG"

# Get a reverse proxy PID
PROXY_PID=$(ps -ef | grep reverse_proxy | grep -v grep | head -1 | awk '{print $2}')
if [ -n "$PROXY_PID" ]; then
    # Kill the proxy
    kill -SIGKILL "$PROXY_PID" 2>/dev/null
    sleep 3
    
    # Check if system still works
    echo "16" | timeout 5 ./client 131 > recovery_test.out 2>&1
    if grep -q "4.0" recovery_test.out; then
        log_test "Process Recovery" "PASS"
    else
        log_test "Process Recovery" "FAIL" "System did not recover from proxy failure"
    fi
else
    log_test "Process Recovery" "FAIL" "Could not find reverse proxy to kill"
fi

# Phase 7: Graceful Shutdown Testing
echo ""
echo "Phase 7: Graceful Shutdown Testing" | tee -a "$TEST_LOG"
echo "===================================" | tee -a "$TEST_LOG"

# Send SIGTSTP to watchdog
if [ -n "$WATCHDOG_PID" ] && kill -0 "$WATCHDOG_PID" 2>/dev/null; then
    kill -SIGTSTP "$WATCHDOG_PID" 2>/dev/null
    sleep 3
    
    # Check if all processes are gone
    REMAINING_PROCESSES=$(ps -ef | grep -E "(watchdog|load_balancer|reverse_proxy|server)" | grep -v grep | wc -l)
    if [ "$REMAINING_PROCESSES" -eq 0 ]; then
        log_test "Graceful Shutdown" "PASS"
    else
        log_test "Graceful Shutdown" "FAIL" "$REMAINING_PROCESSES processes still running"
    fi
    WATCHDOG_PID=""  # Clear PID since we killed it
else
    log_test "Graceful Shutdown" "FAIL" "Watchdog not running"
fi

# Phase 8: Socket Cleanup Testing
echo ""
echo "Phase 8: Socket Cleanup Testing" | tee -a "$TEST_LOG"
echo "================================" | tee -a "$TEST_LOG"

SOCKET_COUNT=$(ls /tmp/server_* /tmp/reverse_proxy_* /tmp/load_balancer 2>/dev/null | wc -l)
if [ "$SOCKET_COUNT" -eq 0 ]; then
    log_test "Socket Cleanup" "PASS"
else
    log_test "Socket Cleanup" "FAIL" "$SOCKET_COUNT socket files remain"
fi

# Test Summary
echo ""
echo "=============================================="
echo "TEST SUMMARY"
echo "=============================================="
echo "Total Passed: $PASS_COUNT" | tee -a "$TEST_LOG"
echo "Total Failed: $FAIL_COUNT" | tee -a "$TEST_LOG"
echo "Success Rate: $(( PASS_COUNT * 100 / (PASS_COUNT + FAIL_COUNT) ))%" | tee -a "$TEST_LOG"
echo ""

if [ "$FAIL_COUNT" -eq 0 ]; then
    echo "🎉 ALL TESTS PASSED! Your system is ready for submission." | tee -a "$TEST_LOG"
    echo ""
    echo "Next Steps:"
    echo "1. Review the detailed test log: $TEST_LOG"
    echo "2. Create your project ZIP file"
    echo "3. Update report.txt with your name and date"
    echo "4. Submit to Moodle"
else
    echo "⚠️  Some tests failed. Please review the issues above." | tee -a "$TEST_LOG"
    echo ""
    echo "Debugging steps:"
    echo "1. Check system.log for detailed error messages"
    echo "2. Review failed test output files"
    echo "3. Run individual tests manually for debugging"
fi

# Cleanup temporary files
rm -f client_*.out recovery_test.out system.log 2>/dev/null

echo ""
echo "Testing completed at $(date)" | tee -a "$TEST_LOG" 
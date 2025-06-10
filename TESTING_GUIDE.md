# CMPE476 Distributed Systems - Comprehensive Testing Guide

## Prerequisites (Ubuntu VM Setup)

### 1. System Requirements
```bash
# Update your Ubuntu system
sudo apt update && sudo apt upgrade -y

# Install essential development tools
sudo apt install build-essential gcc gdb valgrind htop

# Verify GCC version (should support C11)
gcc --version
```

### 2. Project Setup
```bash
# Transfer your project files to Ubuntu VM
# Extract if coming from ZIP
# Navigate to project directory
cd /path/to/your/project

# Verify all files are present
ls -la
# Expected files: watchdog.c, load_balancer.c, reverse_proxy.c, server.c, client.c, Makefile
```

## Testing Protocol

### Phase 1: Basic Compilation and Setup Testing

#### Test 1.1: Compilation Verification
```bash
# Clean and compile
make clean
make

# Verify all executables are created
ls -la watchdog load_balancer reverse_proxy server client
echo "✅ All executables should exist with execute permissions"
```

#### Test 1.2: Help/Usage Testing
```bash
# Test command-line argument validation
./server
./reverse_proxy
./client
echo "✅ Should show usage messages for missing arguments"
```

### Phase 2: Basic Functionality Testing

#### Test 2.1: Single Client Test
**Terminal 1 (Main System):**
```bash
./watchdog
```
**Expected Output:**
```
[Watchdog]: Started
[Watchdog]: Creating Load Balancer
[Load Balancer]: Started
[Load Balancer]: Creating Reverse Proxy #1
[Load Balancer]: Creating Reverse Proxy #2
[Reverse Proxy #1]: Started
[Reverse Proxy #2]: Started
[Reverse Proxy #1]: Creating 3 servers
[Server #1]: Started
[Server #2]: Started
[Server #3]: Started
[Reverse Proxy #2]: Creating 3 servers
[Server #4]: Started
[Server #5]: Started
[Server #6]: Started
```

**Terminal 2 (Client Test):**
```bash
# Test odd client ID (should go to Proxy #1)
./client 107
# Input: 16
# Expected: Result: 4.0

# Check main terminal for logs:
# [Load balancer]: Request from Client #107. Forwarding to Proxy #1
# [Reverse Proxy #1]: Request from Client #107. Forwarding to Server #X
# [Server #X]: Received the value 16.0 from Client #107. Returning 4.0.
```

#### Test 2.2: Load Balancing Verification
**Terminal 3:**
```bash
# Test even client ID (should go to Proxy #2)
./client 112
# Input: 9
# Expected: Result: 3.0
```

**Terminal 4:**
```bash
# Test another odd client (should go to Proxy #1)
./client 101
# Input: 25
# Expected: Result: 5.0
```

#### Test 2.3: Input Validation Testing
```bash
# Test negative value handling
./client 101
# Input: -4
# Expected: Result: -1.0

# Check main terminal for validation log:
# [Reverse Proxy #1]: Illegal request from Client #101. Returning -1.
```

### Phase 3: Process Management Testing

#### Test 3.1: Process Discovery
```bash
# In a new terminal, check running processes
ps -ef | grep -E "(watchdog|load_balancer|reverse_proxy|server)" | grep -v grep

# Expected: 9 processes total (1 watchdog + 1 load_balancer + 2 proxies + 6 servers)
# Document the PIDs for later use
```

#### Test 3.2: Process Recovery Testing
```bash
# Find a reverse proxy PID
PROXY_PID=$(ps -ef | grep reverse_proxy | grep -v grep | head -1 | awk '{print $2}')
echo "Killing reverse proxy with PID: $PROXY_PID"

# Kill one reverse proxy to test watchdog recovery
kill -SIGKILL $PROXY_PID

# Check main terminal - should show:
# [Watchdog]: Reverse Proxy has died. Re-creating.
# Followed by new proxy and server startup messages

# Verify new processes are created
ps -ef | grep -E "(reverse_proxy|server)" | grep -v grep
```

#### Test 3.3: Multiple Process Failure Testing
```bash
# Kill multiple servers to test proxy resilience
SERVER_PIDS=$(ps -ef | grep server | grep -v grep | awk '{print $2}' | head -2)
for pid in $SERVER_PIDS; do
    echo "Killing server PID: $pid"
    kill -SIGKILL $pid
done

# Test if system can still handle requests
./client 113
# Input: 4
# Should still work as proxies respawn servers
```

### Phase 4: Stress and Concurrent Testing

#### Test 4.1: Multiple Concurrent Clients
Create a test script:
```bash
cat > concurrent_test.sh << 'EOF'
#!/bin/bash
echo "Starting concurrent client tests..."

# Launch multiple clients simultaneously
for i in {101..110}; do
    echo "25" | ./client $i &
    sleep 0.1
done

wait
echo "All concurrent tests completed"
EOF

chmod +x concurrent_test.sh
./concurrent_test.sh
```

#### Test 4.2: Load Distribution Testing
```bash
cat > load_test.sh << 'EOF'
#!/bin/bash
echo "Testing load distribution..."

# Test odd clients (should go to Proxy #1)
echo "Testing odd clients:"
for i in {101..105}; do
    echo "16" | ./client $i
    sleep 0.5
done

echo -e "\nTesting even clients:"
# Test even clients (should go to Proxy #2)
for i in {102..106}; do
    echo "9" | ./client $i
    sleep 0.5
done
EOF

chmod +x load_test.sh
./load_test.sh
```

### Phase 5: Graceful Shutdown Testing

#### Test 5.1: Normal Shutdown
```bash
# Find watchdog PID
WATCHDOG_PID=$(ps -ef | grep watchdog | grep -v grep | awk '{print $2}')
echo "Watchdog PID: $WATCHDOG_PID"

# Send SIGTSTP for graceful shutdown
kill -SIGTSTP $WATCHDOG_PID

# Expected output in main terminal:
# [Watchdog]: Received SIGTSTP from user. Terminating all processes.
# [Server #1]: Received SIGTERM from watchdog. Terminating.
# [Server #2]: Received SIGTERM from watchdog. Terminating.
# ... (all servers)
# [Reverse Proxy #1]: Received SIGTERM from watchdog. Terminating.
# [Reverse Proxy #2]: Received SIGTERM from watchdog. Terminating.
# [Watchdog]: All processes terminated. Good bye.
```

#### Test 5.2: Cleanup Verification
```bash
# Verify no processes remain
ps -ef | grep -E "(watchdog|load_balancer|reverse_proxy|server)" | grep -v grep
# Should return no results

# Check for leftover socket files
ls -la /tmp/server_* /tmp/reverse_proxy_* /tmp/load_balancer 2>/dev/null
# Should show "No such file or directory"
```

### Phase 6: Memory and Performance Testing

#### Test 6.1: Memory Leak Detection (Optional)
```bash
# Install valgrind if not present
sudo apt install valgrind

# Test server component for memory leaks
valgrind --leak-check=full --show-leak-kinds=all ./server 1 &
SERVER_PID=$!

# Send a test request
echo "16" | ./client 107

# Kill server and check valgrind output
kill $SERVER_PID
```

#### Test 6.2: System Resource Monitoring
```bash
# Monitor system resources during operation
htop &  # In one terminal

# Start the system
./watchdog &

# Run stress test
for i in {1..50}; do
    echo "100" | ./client $((100 + i)) &
done

# Monitor CPU and memory usage in htop
```

## Test Results Documentation Template

### Test Log Template
```
=== CMPE476 Distributed Systems Testing Report ===
Date: [DATE]
Tester: [YOUR NAME]
VM: Ubuntu [VERSION]

Phase 1 - Compilation: [ PASS / FAIL ]
  - All executables created: [ YES / NO ]
  - No compilation errors: [ YES / NO ]

Phase 2 - Basic Functionality: [ PASS / FAIL ]
  - Single client test: [ PASS / FAIL ]
  - Load balancing (odd→proxy1): [ PASS / FAIL ]
  - Load balancing (even→proxy2): [ PASS / FAIL ]
  - Input validation (negative): [ PASS / FAIL ]

Phase 3 - Process Management: [ PASS / FAIL ]
  - Process discovery: [ PASS / FAIL ]
  - Process recovery: [ PASS / FAIL ]
  - Multiple failure handling: [ PASS / FAIL ]

Phase 4 - Stress Testing: [ PASS / FAIL ]
  - Concurrent clients: [ PASS / FAIL ]
  - Load distribution: [ PASS / FAIL ]

Phase 5 - Graceful Shutdown: [ PASS / FAIL ]
  - SIGTSTP handling: [ PASS / FAIL ]
  - Complete cleanup: [ PASS / FAIL ]

Issues Encountered:
[List any problems and their solutions]

Performance Notes:
[CPU/Memory usage observations]
```

## Troubleshooting Common Issues

### Issue 1: "Address already in use"
```bash
# Solution: Clean up socket files
rm -f /tmp/server_* /tmp/reverse_proxy_* /tmp/load_balancer
```

### Issue 2: "Permission denied"
```bash
# Solution: Make executables
chmod +x watchdog load_balancer reverse_proxy server client
```

### Issue 3: Processes not starting
```bash
# Check system limits
ulimit -a
# Increase if needed:
ulimit -n 1024  # File descriptors
```

### Issue 4: Client connection failures
```bash
# Check if system is fully started
sleep 3  # Wait for all components to initialize
```

## Final Submission Checklist

- [ ] All test phases passed
- [ ] Test log completed
- [ ] No memory leaks detected
- [ ] Graceful shutdown working
- [ ] Load balancing verified
- [ ] Process recovery confirmed
- [ ] Input validation working
- [ ] Performance acceptable
- [ ] Clean compilation
- [ ] All files present for ZIP submission

## Quick Test Command Summary
```bash
# Full test sequence
make clean && make
./watchdog &                    # Terminal 1
echo "16" | ./client 107        # Terminal 2
echo "9" | ./client 112         # Terminal 3
echo "-4" | ./client 101        # Terminal 4
kill -SIGTSTP $(pgrep watchdog) # Shutdown
```

---
**Note**: Run each test phase systematically and document results. This ensures comprehensive verification of your distributed system implementation. 
# Ubuntu VM Testing Guide - Complete Overview

## 🎯 Quick Start for Ubuntu VM Testing

### Step 1: VM Preparation
```bash
# On your Ubuntu VM, run this first:
./prepare_vm.sh
```
This script will:
- Update system packages
- Install development tools (gcc, make, gdb, valgrind, htop)
- Verify C compilation environment
- Check system resources and limits
- Set up project directory structure

### Step 2: Project Transfer
```bash
# Copy all project files to your Ubuntu VM
# If using SCP from your Mac:
scp -r /path/to/project/* username@vm-ip:~/distributed_systems_project/

# Or if using shared folders, just copy the files to your project directory
```

### Step 3: Quick Functionality Test
```bash
# Navigate to project directory
cd ~/distributed_systems_project

# Make scripts executable
chmod +x *.sh

# Run automated tests
./auto_test.sh
```

### Step 4: Manual Testing (if needed)
```bash
# Follow the comprehensive manual testing guide
# Open TESTING_GUIDE.md for detailed step-by-step instructions
```

## 📋 Testing Phases Overview

### Phase 1: Environment Verification ✅
- **Purpose**: Ensure VM is properly configured
- **Time**: 2-3 minutes
- **Command**: `./prepare_vm.sh`
- **Expected**: All development tools installed and working

### Phase 2: Automated System Testing ⚡
- **Purpose**: Complete system functionality verification
- **Time**: 3-5 minutes
- **Command**: `./auto_test.sh`
- **Tests**:
  - ✅ Compilation verification
  - ✅ System startup (9 processes)
  - ✅ Load balancing (odd→proxy1, even→proxy2)
  - ✅ Input validation (negative numbers)
  - ✅ Concurrent client handling
  - ✅ Process recovery (killing and respawning)
  - ✅ Graceful shutdown (SIGTSTP)
  - ✅ Socket cleanup

### Phase 3: Manual Deep Testing 🔍
- **Purpose**: Detailed verification and debugging
- **Time**: 15-20 minutes
- **Guide**: Follow `TESTING_GUIDE.md`
- **Focus Areas**:
  - Process monitoring with `htop` and `ps`
  - Network socket verification
  - Memory leak detection with `valgrind`
  - Stress testing with multiple clients

### Phase 4: Submission Preparation 📦
- **Purpose**: Create final submission package
- **Time**: 2 minutes
- **Command**: `./prepare_submission.sh`
- **Output**: Ready-to-submit ZIP file

## 🚨 Common Ubuntu VM Issues & Solutions

### Issue 1: Permission Denied Errors
```bash
# Solution: Fix permissions
chmod +x watchdog load_balancer reverse_proxy server client
chmod +x *.sh
```

### Issue 2: Socket Address Already in Use
```bash
# Solution: Clean up socket files
rm -f /tmp/server_* /tmp/reverse_proxy_* /tmp/load_balancer
```

### Issue 3: Process Limits
```bash
# Check current limits
ulimit -a

# Increase if needed
ulimit -n 1024  # File descriptors
ulimit -u 2048  # Processes
```

### Issue 4: GCC Version Issues
```bash
# Verify GCC supports C11
gcc --version
# Should be GCC 4.9+ for full C11 support

# If needed, install newer GCC
sudo apt install gcc-9
```

### Issue 5: VM Performance Issues
```bash
# Monitor resources
htop

# Check available memory
free -h

# Check disk space
df -h
```

## 📊 Expected Test Results

### Successful Auto Test Output:
```
==============================================
TEST SUMMARY
==============================================
Total Passed: 8
Total Failed: 0
Success Rate: 100%

🎉 ALL TESTS PASSED! Your system is ready for submission.
```

### System Startup Logs (Expected):
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

### Process Count Verification:
```bash
ps -ef | grep -E "(watchdog|load_balancer|reverse_proxy|server)" | grep -v grep
# Should show exactly 9 processes:
# 1 watchdog + 1 load_balancer + 2 reverse_proxy + 6 server = 9 total
```

## 🎮 Interactive Testing Scenarios

### Scenario 1: Basic Client Testing
```bash
# Terminal 1: Start system
./watchdog

# Terminal 2: Test odd client (→ Proxy 1)
./client 107
# Input: 16
# Expected: Result: 4.0

# Terminal 3: Test even client (→ Proxy 2) 
./client 112
# Input: 9
# Expected: Result: 3.0

# Terminal 4: Test input validation
./client 101
# Input: -4
# Expected: Result: -1.0
```

### Scenario 2: Process Recovery Testing
```bash
# Find and kill a proxy
PROXY_PID=$(ps -ef | grep reverse_proxy | grep -v grep | head -1 | awk '{print $2}')
kill -SIGKILL $PROXY_PID

# System should auto-recover and still work:
./client 113
# Input: 25
# Expected: Result: 5.0
```

### Scenario 3: Graceful Shutdown
```bash
# Find watchdog and send SIGTSTP
WATCHDOG_PID=$(ps -ef | grep watchdog | grep -v grep | awk '{print $2}')
kill -SIGTSTP $WATCHDOG_PID

# Expected: All processes terminate gracefully with proper log messages
```

## 📝 Testing Documentation Template

```
=== Ubuntu VM Testing Report ===
VM OS: Ubuntu [version]
Date: [test date]
Time: [test duration]

Environment Setup: [ PASS / FAIL ]
Automated Tests: [ PASS / FAIL ]
Manual Verification: [ PASS / FAIL ]
Performance: [ GOOD / ACCEPTABLE / POOR ]

Notes:
- [Any specific observations]
- [Performance metrics if relevant]
- [Issues encountered and resolved]

Final Status: [ READY FOR SUBMISSION / NEEDS FIXES ]
```

## 🎯 Final Checklist for Ubuntu VM Testing

### Before Testing:
- [ ] Ubuntu VM is running and accessible
- [ ] All project files transferred to VM
- [ ] VM preparation script executed successfully
- [ ] Internet access available for package installation

### During Testing:
- [ ] Automated tests pass 100%
- [ ] Manual verification completed
- [ ] Performance is acceptable
- [ ] No resource limit issues
- [ ] All components start and stop properly

### After Testing:
- [ ] report.txt updated with your information
- [ ] Submission ZIP file created
- [ ] Final compilation test passed
- [ ] Ready for Moodle submission

## ⚡ Time Management for Testing

**Total Estimated Time: 30-45 minutes**

- VM Setup: 5 minutes
- Automated Testing: 5 minutes  
- Manual Verification: 15-20 minutes
- Documentation: 5 minutes
- Submission Prep: 5 minutes

## 🆘 Emergency Debugging

If automated tests fail:

1. **Check system.log**: `cat system.log`
2. **Verify processes**: `ps -ef | grep -E "(watchdog|load_balancer|reverse_proxy|server)"`
3. **Check sockets**: `ls -la /tmp/server_* /tmp/reverse_proxy_* /tmp/load_balancer`
4. **Review client output**: `cat client_*.out`
5. **Manual step-by-step**: Follow TESTING_GUIDE.md Phase 2

## 🏁 Success Indicators

Your system is working correctly when:
- ✅ All 8 automated tests pass
- ✅ 9 processes start correctly (1+1+2+6)
- ✅ Load balancing works (odd→proxy1, even→proxy2)
- ✅ Input validation catches negative numbers
- ✅ Process recovery works after killing components
- ✅ Graceful shutdown terminates all processes
- ✅ No socket files remain after shutdown
- ✅ System handles concurrent clients properly

---

**🎉 You're ready to demonstrate a fully functional distributed system!** 
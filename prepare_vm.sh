#!/bin/bash

# CMPE476 Distributed Systems - Ubuntu VM Preparation Script
# This script prepares the Ubuntu VM environment for testing

echo "=============================================="
echo "CMPE476 Distributed Systems - VM Setup"
echo "=============================================="

# Update system
echo "Updating system packages..."
sudo apt update && sudo apt upgrade -y

# Install development tools
echo "Installing development tools..."
sudo apt install -y build-essential gcc gdb valgrind htop tree

# Verify GCC installation
echo ""
echo "Verifying development environment:"
echo "GCC Version:"
gcc --version | head -1

echo ""
echo "Make Version:"
make --version | head -1

# Check system resources
echo ""
echo "System Information:"
echo "Memory: $(free -h | grep '^Mem' | awk '{print $2}')"
echo "CPU: $(nproc) cores"
echo "Disk space: $(df -h / | tail -1 | awk '{print $4}') available"

# Create project directory if needed
echo ""
echo "Setting up project directory..."
if [ ! -d "~/distributed_systems_project" ]; then
    mkdir -p ~/distributed_systems_project
    echo "Created ~/distributed_systems_project"
else
    echo "Project directory already exists"
fi

# Set proper permissions for common directories
echo ""
echo "Setting up permissions..."
chmod 755 ~ 2>/dev/null || true
mkdir -p ~/.local/bin 2>/dev/null || true

# Check /tmp directory permissions (needed for Unix sockets)
if [ -w "/tmp" ]; then
    echo "✅ /tmp directory is writable (needed for Unix sockets)"
else
    echo "❌ /tmp directory is not writable - this may cause issues"
fi

# Check ulimits
echo ""
echo "System Limits:"
echo "Max file descriptors: $(ulimit -n)"
echo "Max processes: $(ulimit -u)"

# If limits are too low, suggest increases
if [ "$(ulimit -n)" -lt 1024 ]; then
    echo "⚠️  File descriptor limit is low. Consider increasing:"
    echo "   ulimit -n 1024"
fi

# Create a quick test C program to verify compilation
echo ""
echo "Testing C compilation..."
cat > test_compile.c << 'EOF'
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>

int main() {
    printf("C compilation and socket headers work!\n");
    return 0;
}
EOF

if gcc -std=c11 -Wall -o test_compile test_compile.c; then
    ./test_compile
    rm -f test_compile test_compile.c
    echo "✅ C compilation environment is working"
else
    echo "❌ C compilation failed - check your development tools"
    rm -f test_compile test_compile.c
    exit 1
fi

echo ""
echo "=============================================="
echo "VM PREPARATION COMPLETE"
echo "=============================================="
echo ""
echo "Your Ubuntu VM is ready for the distributed systems project!"
echo ""
echo "Next steps:"
echo "1. Copy your project files to ~/distributed_systems_project/"
echo "2. Navigate to the project directory: cd ~/distributed_systems_project"
echo "3. Run: chmod +x *.sh"
echo "4. Start testing with: ./auto_test.sh"
echo ""
echo "For manual testing, follow the guide in TESTING_GUIDE.md"
echo ""
echo "Useful commands for monitoring:"
echo "  - htop          # Monitor system resources"
echo "  - ps -ef        # List all processes"
echo "  - df -h         # Check disk space"
echo "  - free -h       # Check memory usage" 
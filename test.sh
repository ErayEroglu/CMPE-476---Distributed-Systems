#!/bin/bash

# Test script for the distributed system project
echo "=== Distributed Systems Project Test ==="
echo "Compiling all components..."

make clean && make

if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi

echo "All components compiled successfully!"
echo ""
echo "=== Testing Instructions ==="
echo "1. Run './watchdog' in this terminal"
echo "2. In separate terminals, run:"
echo "   ./client 107  (odd client - goes to proxy 1)"
echo "   ./client 112  (even client - goes to proxy 2)"
echo "   ./client 101  (with negative value for error testing)"
echo ""
echo "3. To test process recovery:"
echo "   ps -ef | grep reverse_proxy | grep -v grep"
echo "   kill -SIGKILL [pid_of_one_proxy]"
echo ""
echo "4. To shutdown gracefully:"
echo "   ps -ef | grep watchdog | grep -v grep"
echo "   kill -SIGTSTP [watchdog_pid]"
echo ""
echo "Starting system in 5 seconds..."
sleep 5

./watchdog 
# CMPE476 Distributed Systems Project

## Overview
This project implements a distributed system with load balancing and reverse proxy functionality using C and Unix domain sockets.

## Architecture
```
Client → Load Balancer → Reverse Proxy → Server
```

### Components:
- **Watchdog**: Parent process managing all others
- **Load Balancer**: Routes requests based on client ID (odd→proxy1, even→proxy2)
- **Reverse Proxies (2)**: Validate requests and randomly select servers
- **Servers (6)**: Calculate square roots (3 per proxy)
- **Clients**: Send requests with floating point values

## Compilation
```bash
make              # Compile all components
make clean        # Remove binaries
make watchdog     # Compile specific component
```

## Usage

### 1. Start the System
```bash
./watchdog
```

### 2. Run Clients (in separate terminals)
```bash
./client 107      # Odd client → Proxy 1
./client 112      # Even client → Proxy 2
./client 101      # Test with negative value
```

### 3. Test Process Recovery
```bash
# Find reverse proxy PID
ps -ef | grep reverse_proxy | grep -v grep

# Kill one proxy to test watchdog recovery
kill -SIGKILL [proxy_pid]
```

### 4. Graceful Shutdown
```bash
# Find watchdog PID
ps -ef | grep watchdog | grep -v grep

# Send SIGTSTP to initiate shutdown
kill -SIGTSTP [watchdog_pid]
```

## Expected Output Example
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
[Load balancer]: Request from Client #107. Forwarding to Proxy #1
[Reverse Proxy #1]: Request from Client #107. Forwarding to Server #2
[Server #2]: Received the value 16.0 from Client #107. Returning 4.0.
```

## Features
- Hash-based load balancing (client ID modulo 2)
- Input validation (non-negative values only)
- Random server selection within proxies
- Process failure detection and recovery
- Graceful shutdown with signal cascading
- Comprehensive logging with process identification

## Implementation Details
- **Language**: C (C11 standard)
- **Communication**: Unix domain sockets
- **Process Management**: fork() and exec()
- **Signal Handling**: SIGCHLD, SIGTERM, SIGTSTP
- **Synchronization**: select() with timeouts

## Testing
Use the provided test script:
```bash
./test.sh
```

## Files
- `watchdog.c` - Main process supervisor
- `load_balancer.c` - Request routing component
- `reverse_proxy.c` - Input validation and server selection
- `server.c` - Square root calculation service
- `client.c` - Test client application
- `Makefile` - Build configuration
- `report.pdf` - LLM usage documentation 
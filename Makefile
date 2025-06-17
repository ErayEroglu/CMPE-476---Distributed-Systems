CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -pthread
TARGETS = watchdog load_balancer reverse_proxy server client

.PHONY: all clean $(TARGETS)

all: $(TARGETS)

watchdog: watchdog.c
	$(CC) $(CFLAGS) -o $@ $<

load_balancer: load_balancer.c
	$(CC) $(CFLAGS) -o $@ $<

reverse_proxy: reverse_proxy.c
	$(CC) $(CFLAGS) -o $@ $<

server: server.c
	$(CC) $(CFLAGS) -o $@ $< -lm

client: client.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf client.dSYM load_balancer.dSYM reverse_proxy.dSYM server.dSYM watchdog.dSYM
	rm -f $(TARGETS) *.o

install: all
	@echo "All components compiled successfully"

test: all
	@echo "Running basic compilation test..."
	@echo "All executables created successfully" 
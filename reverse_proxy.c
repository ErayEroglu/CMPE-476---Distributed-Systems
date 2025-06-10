#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>

#define PROXY_SOCKET_BASE "/tmp/reverse_proxy_"
#define SERVER_SOCKET_BASE "/tmp/server_"
#define BUFFER_SIZE 256

static int proxy_id;
static int proxy_socket = -1;
static volatile sig_atomic_t should_exit = 0;

typedef struct {
    int client_id;
    double value;
} request_t;

typedef struct {
    double result;
} response_t;

void signal_handler(int sig) {
    if (sig == SIGTERM) {
        printf("[Reverse Proxy #%d]: Received SIGTERM from watchdog. Terminating.\n", proxy_id);
        should_exit = 1;
        if (proxy_socket != -1) {
            close(proxy_socket);
        }
    }
}

void setup_signals() {
    struct sigaction sa_term;
    
    // Setup SIGTERM handler
    sa_term.sa_handler = signal_handler;
    sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = 0;
    sigaction(SIGTERM, &sa_term, NULL);
}

int create_proxy_socket() {
    char socket_path[256];
    snprintf(socket_path, sizeof(socket_path), "%s%d", PROXY_SOCKET_BASE, proxy_id);
    
    // Remove existing socket file
    unlink(socket_path);
    
    proxy_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (proxy_socket == -1) {
        perror("socket");
        return -1;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, socket_path);
    
    if (bind(proxy_socket, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(proxy_socket);
        return -1;
    }
    
    if (listen(proxy_socket, 5) == -1) {
        perror("listen");
        close(proxy_socket);
        return -1;
    }
    
    return 0;
}

int connect_to_server(int server_id) {
    char socket_path[256];
    snprintf(socket_path, sizeof(socket_path), "%s%d", SERVER_SOCKET_BASE, server_id);
    
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return -1;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, socket_path);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock);
        return -1;
    }
    
    return sock;
}

void process_request(int client_sock) {
    request_t req;
    response_t resp;
    
    ssize_t bytes_read = recv(client_sock, &req, sizeof(req), 0);
    if (bytes_read != sizeof(req)) {
        printf("[Reverse Proxy #%d]: Error reading request\n", proxy_id);
        return;
    }
    
    // Validate request (non-negative value)
    if (req.value < 0) {
        printf("[Reverse Proxy #%d]: Illegal request from Client #%d. Returning -1.\n", 
               proxy_id, req.client_id);
        resp.result = -1.0;
        send(client_sock, &resp, sizeof(resp), 0);
        return;
    }
    
    // Randomly select a server (1-3 for this proxy)
    int server_index = rand() % 3;
    int server_id = (proxy_id - 1) * 3 + server_index + 1;
    
    printf("[Reverse Proxy #%d]: Request from Client #%d. Forwarding to Server #%d\n", 
           proxy_id, req.client_id, server_id);
    
    // Connect to selected server
    int server_sock = connect_to_server(server_id);
    if (server_sock == -1) {
        printf("[Reverse Proxy #%d]: Failed to connect to Server #%d\n", proxy_id, server_id);
        resp.result = -1.0;
        send(client_sock, &resp, sizeof(resp), 0);
        return;
    }
    
    // Forward request to server
    if (send(server_sock, &req, sizeof(req), 0) == -1) {
        printf("[Reverse Proxy #%d]: Error sending to server\n", proxy_id);
        close(server_sock);
        return;
    }
    
    // Receive response from server
    if (recv(server_sock, &resp, sizeof(resp), 0) != sizeof(resp)) {
        printf("[Reverse Proxy #%d]: Error receiving from server\n", proxy_id);
        close(server_sock);
        return;
    }
    
    close(server_sock);
    
    // Send response back to load balancer
    if (send(client_sock, &resp, sizeof(resp), 0) == -1) {
        printf("[Reverse Proxy #%d]: Error sending response\n", proxy_id);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <proxy_id>\n", argv[0]);
        exit(1);
    }
    
    proxy_id = atoi(argv[1]);
    srand(time(NULL) + proxy_id); // Seed random number generator
    
    setup_signals();
    
    printf("[Reverse Proxy #%d]: Started\n", proxy_id);
    
    if (create_proxy_socket() == -1) {
        exit(1);
    }
    
    while (!should_exit) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(proxy_socket, &readfds);
        
        struct timeval timeout = {1, 0}; // 1 second timeout
        
        int ready = select(proxy_socket + 1, &readfds, NULL, NULL, &timeout);
        if (ready == -1) {
            if (errno == EINTR) continue; // Interrupted by signal
            perror("select");
            break;
        }
        
        if (ready > 0 && FD_ISSET(proxy_socket, &readfds)) {
            int client_sock = accept(proxy_socket, NULL, NULL);
            if (client_sock == -1) {
                if (errno == EINTR) continue;
                perror("accept");
                continue;
            }
            
            process_request(client_sock);
            close(client_sock);
        }
    }
    
    // Clean up
    if (proxy_socket != -1) {
        close(proxy_socket);
        char socket_path[256];
        snprintf(socket_path, sizeof(socket_path), "%s%d", PROXY_SOCKET_BASE, proxy_id);
        unlink(socket_path);
    }
    
    return 0;
} 
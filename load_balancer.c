#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/time.h>

#define LOAD_BALANCER_SOCKET "/tmp/load_balancer"
#define PROXY_SOCKET_BASE "/tmp/reverse_proxy_"
#define BUFFER_SIZE 256

static int lb_socket = -1;
static volatile sig_atomic_t should_exit = 0;

typedef struct {
    int client_id;
    double value;
} request_t;

typedef struct {
    double result;
} response_t;

// prompt : Implement signal handler for SIGTERM. 
void signal_handler(int sig) {
    if (sig == SIGTERM) {
        printf("[Load Balancer]: Received SIGTERM from watchdog. Terminating.\n");
        should_exit = 1;
        if (lb_socket != -1) {
            close(lb_socket);
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

int create_load_balancer_socket() {
    // Remove existing socket file
    unlink(LOAD_BALANCER_SOCKET);
    
    lb_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (lb_socket == -1) {
        perror("socket");
        return -1;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, LOAD_BALANCER_SOCKET);
    
    if (bind(lb_socket, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(lb_socket);
        return -1;
    }
    
    if (listen(lb_socket, 10) == -1) {
        perror("listen");
        close(lb_socket);
        return -1;
    }
    
    return 0;
}

int connect_to_proxy(int proxy_id) {
    char socket_path[256];
    snprintf(socket_path, sizeof(socket_path), "%s%d", PROXY_SOCKET_BASE, proxy_id);
    
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
        printf("[Load Balancer]: Error reading request\n");
        return;
    }
    
    // Hash function: odd client IDs go to proxy 1, even to proxy 2
    int proxy_id = (req.client_id % 2 == 1) ? 1 : 2;
    
    printf("[Load balancer]: Request from Client #%d. Forwarding to Proxy #%d\n", 
           req.client_id, proxy_id);
    
    // Connect to selected proxy
    int proxy_sock = connect_to_proxy(proxy_id);
    if (proxy_sock == -1) {
        printf("[Load Balancer]: Failed to connect to Proxy #%d\n", proxy_id);
        resp.result = -1.0;
        send(client_sock, &resp, sizeof(resp), 0);
        return;
    }
    
    // Forward request to proxy
    if (send(proxy_sock, &req, sizeof(req), 0) == -1) {
        printf("[Load Balancer]: Error sending to proxy\n");
        close(proxy_sock);
        return;
    }
    
    // Receive response from proxy
    if (recv(proxy_sock, &resp, sizeof(resp), 0) != sizeof(resp)) {
        printf("[Load Balancer]: Error receiving from proxy\n");
        close(proxy_sock);
        return;
    }
    
    close(proxy_sock);
    
    // Send response back to client
    if (send(client_sock, &resp, sizeof(resp), 0) == -1) {
        printf("[Load Balancer]: Error sending response to client\n");
    }
}

int main() {
    setup_signals();
    
    printf("[Load Balancer]: Started\n");
    
    if (create_load_balancer_socket() == -1) {
        exit(1);
    }
    
    while (!should_exit) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(lb_socket, &readfds);
        
        struct timeval timeout = {1, 0}; // 1 second timeout
        
        int ready = select(lb_socket + 1, &readfds, NULL, NULL, &timeout);
        if (ready == -1) {
            if (errno == EINTR) continue; // Interrupted by signal
            perror("select");
            break;
        }
        
        if (ready > 0 && FD_ISSET(lb_socket, &readfds)) {
            int client_sock = accept(lb_socket, NULL, NULL);
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
    if (lb_socket != -1) {
        close(lb_socket);
        unlink(LOAD_BALANCER_SOCKET);
    }
    
    return 0;
} 
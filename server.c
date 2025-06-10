#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <errno.h>

#define SOCKET_PATH_BASE "/tmp/server_"
#define BUFFER_SIZE 256

static int server_id;
static int server_socket = -1;
static volatile sig_atomic_t should_exit = 0;

typedef struct {
    int client_id;
    double value;
} request_t;

typedef struct {
    double result;
} response_t;

// prompt : Implement signal handler for SIGTERM. s
void signal_handler(int sig) {
    if (sig == SIGTERM) {
        printf("[Server #%d]: Received SIGTERM from watchdog. Terminating.\n", server_id);
        should_exit = 1;
        if (server_socket != -1) {
            close(server_socket);
        }
    }
}

void setup_signals() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
}

int create_server_socket() {
    char socket_path[256];
    snprintf(socket_path, sizeof(socket_path), "%s%d", SOCKET_PATH_BASE, server_id);
    
    // Remove existing socket file
    unlink(socket_path);
    
    server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        return -1;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, socket_path);
    
    if (bind(server_socket, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server_socket);
        return -1;
    }
    
    if (listen(server_socket, 5) == -1) {
        perror("listen");
        close(server_socket);
        return -1;
    }
    
    return 0;
}

void process_request(int client_sock) {
    request_t req;
    response_t resp;
    
    ssize_t bytes_read = recv(client_sock, &req, sizeof(req), 0);
    if (bytes_read != sizeof(req)) {
        printf("[Server #%d]: Error reading request\n", server_id);
        return;
    }
    
    // Calculate square root
    resp.result = sqrt(req.value);
    
    printf("[Server #%d]: Received the value %.1f from Client #%d. Returning %.1f.\n", 
           server_id, req.value, req.client_id, resp.result);
    
    // Send response back
    if (send(client_sock, &resp, sizeof(resp), 0) == -1) {
        printf("[Server #%d]: Error sending response\n", server_id);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_id>\n", argv[0]);
        exit(1);
    }
    
    server_id = atoi(argv[1]);
    
    setup_signals();
    
    if (create_server_socket() == -1) {
        exit(1);
    }
    
    printf("[Server #%d]: Started\n", server_id);
    
    while (!should_exit) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);
        
        struct timeval timeout = {1, 0}; // 1 second timeout
        
        int ready = select(server_socket + 1, &readfds, NULL, NULL, &timeout);
        if (ready == -1) {
            if (errno == EINTR) continue; // Interrupted by signal
            perror("select");
            break;
        }
        
        if (ready > 0 && FD_ISSET(server_socket, &readfds)) {
            int client_sock = accept(server_socket, NULL, NULL);
            if (client_sock == -1) {
                if (errno == EINTR) continue;
                perror("accept");
                continue;
            }
            
            process_request(client_sock);
            close(client_sock);
        }
    }
    
    if (server_socket != -1) {
        close(server_socket);
        char socket_path[256];
        snprintf(socket_path, sizeof(socket_path), "%s%d", SOCKET_PATH_BASE, server_id);
        unlink(socket_path);
    }
    
    return 0;
} 
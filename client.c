#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

#define LOAD_BALANCER_SOCKET "/tmp/load_balancer"

typedef struct {
    int client_id;
    double value;
} request_t;

typedef struct {
    double result;
} response_t;

int connect_to_load_balancer() {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return -1;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, LOAD_BALANCER_SOCKET);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock);
        return -1;
    }
    
    return sock;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <client_id>\n", argv[0]);
        exit(1);
    }
    
    int client_id = atoi(argv[1]);
    printf("This is client #%d\n", client_id);
    
    // Get input from user
    double value;
    printf("Enter a non-negative float: ");
    if (scanf("%lf", &value) != 1) {
        printf("Invalid input\n");
        exit(1);
    }
    
    // Connect to load balancer
    int sock = connect_to_load_balancer();
    if (sock == -1) {
        printf("Failed to connect to load balancer\n");
        exit(1);
    }
    
    // Prepare request
    request_t req;
    req.client_id = client_id;
    req.value = value;
    
    // Send request
    if (send(sock, &req, sizeof(req), 0) == -1) {
        perror("send");
        close(sock);
        exit(1);
    }
    
    // Receive response
    response_t resp;
    ssize_t bytes_received = recv(sock, &resp, sizeof(resp), 0);
    if (bytes_received != sizeof(resp)) {
        printf("Error receiving response\n");
        close(sock);
        exit(1);
    }
    
    // Display result
    printf("      Result: %.1f\n", resp.result);
    
    close(sock);
    return 0;
} 
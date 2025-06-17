#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

static pid_t load_balancer_pid = 0;
static pid_t reverse_proxy_pids[2] = {0, 0};
static pid_t server_pids[6] = {0, 0, 0, 0, 0, 0};
static volatile sig_atomic_t should_exit = 0;

void sigchld_handler(int sig) {
    (void)sig; // Unused parameter
    
    pid_t pid;
    int status;
    
    // prompt : All children of the watchdog must be killed when the watchdog is killed. That includes the load balancer, reverse proxies, and servers. Update the code to ensure this.
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (pid == load_balancer_pid) {
            if (!should_exit) {
                printf("[Watchdog]: Load balancer has died. Re-creating.\n");
                sleep(1); // Brief delay before respawning
                
                // Respawn load balancer
                load_balancer_pid = fork();
                if (load_balancer_pid == 0) {
                    execl("./load_balancer", "load_balancer", NULL);
                    perror("execl load_balancer");
                    exit(1);
                } else if (load_balancer_pid == -1) {
                    perror("fork load_balancer");
                    exit(1);
                } else {
                    printf("[Watchdog]: Load balancer respawned with PID %d\n", load_balancer_pid);
                }
            }
        } else {
            // Check reverse proxies
            for (int i = 0; i < 2; i++) {
                if (pid == reverse_proxy_pids[i] && !should_exit) {
                    printf("[Watchdog]: Reverse Proxy has died. Re-creating.\n");
                    sleep(1); // Brief delay before respawning
                    
                    // Respawn reverse proxy
                    reverse_proxy_pids[i] = fork();
                    if (reverse_proxy_pids[i] == 0) {
                        char proxy_id_str[16];
                        snprintf(proxy_id_str, sizeof(proxy_id_str), "%d", i + 1);
                        execl("./reverse_proxy", "reverse_proxy", proxy_id_str, NULL);
                        perror("execl reverse_proxy");
                        exit(1);
                    } else if (reverse_proxy_pids[i] == -1) {
                        perror("fork reverse_proxy");
                        exit(1);
                    } else {
                        printf("[Watchdog]: Reverse Proxy respawned with PID %d\n", reverse_proxy_pids[i]);
                    }
                    return;
                }
            }
            
            // Check servers
            for (int i = 0; i < 6; i++) {
                if (pid == server_pids[i] && !should_exit) {
                    printf("[Watchdog]: Server has died. Re-creating.\n");
                    sleep(1); // Brief delay before respawning
                    
                    // Respawn server
                    server_pids[i] = fork();
                    if (server_pids[i] == 0) {
                        char server_id_str[16];
                        snprintf(server_id_str, sizeof(server_id_str), "%d", i + 1);
                        execl("./server", "server", server_id_str, NULL);
                        perror("execl server");
                        exit(1);
                    } else if (server_pids[i] == -1) {
                        perror("fork server");
                        exit(1);
                    }
                    return;
                }
            }
            
            if (!should_exit) {
                printf("[Watchdog]: Unknown child process %d has died.\n", pid);
            }
        }
    }
}

// prompt: Implement signal handler for SIGINT and SIGTSTP. 
void sigint_handler(int sig) {
    (void)sig; // Unused parameter
    
    printf("\n[Watchdog]: Received SIGINT. Terminating all processes.\n");
    should_exit = 1;
    
    // Send SIGTERM to all processes
    if (load_balancer_pid > 0) {
        kill(load_balancer_pid, SIGTERM);
    }
    
    for (int i = 0; i < 2; i++) {
        if (reverse_proxy_pids[i] > 0) {
            kill(reverse_proxy_pids[i], SIGTERM);
        }
    }
    
    for (int i = 0; i < 6; i++) {
        if (server_pids[i] > 0) {
            kill(server_pids[i], SIGTERM);
        }
    }
    
    // Wait for all processes to terminate
    if (load_balancer_pid > 0) {
        waitpid(load_balancer_pid, NULL, 0);
    }
    
    for (int i = 0; i < 2; i++) {
        if (reverse_proxy_pids[i] > 0) {
            waitpid(reverse_proxy_pids[i], NULL, 0);
        }
    }
    
    for (int i = 0; i < 6; i++) {
        if (server_pids[i] > 0) {
            waitpid(server_pids[i], NULL, 0);
        }
    }
    
    printf("[Watchdog]: All processes terminated. Good bye.\n");
    exit(0);
}

void sigtstp_handler(int sig) {
    (void)sig; // Unused parameter
    
    printf("\n[Watchdog]: Received SIGTSTP. Terminating all processes.\n");
    should_exit = 1;
    
    // Send SIGTERM to all processes
    if (load_balancer_pid > 0) {
        kill(load_balancer_pid, SIGTERM);
    }
    
    for (int i = 0; i < 2; i++) {
        if (reverse_proxy_pids[i] > 0) {
            kill(reverse_proxy_pids[i], SIGTERM);
        }
    }
    
    for (int i = 0; i < 6; i++) {
        if (server_pids[i] > 0) {
            kill(server_pids[i], SIGTERM);
        }
    }
    
    // Wait for all processes to terminate
    if (load_balancer_pid > 0) {
        waitpid(load_balancer_pid, NULL, 0);
    }
    
    for (int i = 0; i < 2; i++) {
        if (reverse_proxy_pids[i] > 0) {
            waitpid(reverse_proxy_pids[i], NULL, 0);
        }
    }
    
    for (int i = 0; i < 6; i++) {
        if (server_pids[i] > 0) {
            waitpid(server_pids[i], NULL, 0);
        }
    }
    
    printf("[Watchdog]: All processes terminated. Good bye.\n");
    exit(0);
}

void setup_signals() {
    struct sigaction sa_chld, sa_int, sa_tstp;
    
    // Setup SIGCHLD handler
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
        perror("sigaction SIGCHLD");
        exit(1);
    }
    
    // Setup SIGINT handler (Ctrl+C)
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("sigaction SIGINT");
        exit(1);
    }
    
    // Setup SIGTSTP handler (Ctrl+Z) 
    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = 0;
    if (sigaction(SIGTSTP, &sa_tstp, NULL) == -1) {
        perror("sigaction SIGTSTP");
        exit(1);
    }
}

// prompt: Implement methods to spawn the load balancer, reverse proxies, and servers. 
void create_load_balancer() {
    printf("[Watchdog]: Creating Load Balancer\n");
    
    load_balancer_pid = fork();
    if (load_balancer_pid == 0) {
        // Child process - run load balancer
        execl("./load_balancer", "load_balancer", NULL);
        perror("execl load_balancer");
        exit(1);
    } else if (load_balancer_pid == -1) {
        perror("fork load_balancer");
        exit(1);
    }
    
    // Give load balancer time to start
    sleep(1);
}

void create_reverse_proxies() {
    for (int i = 0; i < 2; i++) {
        printf("[Watchdog]: Creating Reverse Proxy #%d\n", i + 1);
        
        reverse_proxy_pids[i] = fork();
        if (reverse_proxy_pids[i] == 0) {
            // Child process - run reverse proxy
            char proxy_id_str[16];
            snprintf(proxy_id_str, sizeof(proxy_id_str), "%d", i + 1);
            execl("./reverse_proxy", "reverse_proxy", proxy_id_str, NULL);
            perror("execl reverse_proxy");
            exit(1);
        } else if (reverse_proxy_pids[i] == -1) {
            perror("fork reverse_proxy");
            exit(1);
        }
    }
    
    // Give proxies time to start
    sleep(1);
}

void create_servers() {
    for (int i = 0; i < 6; i++) {
        printf("[Watchdog]: Creating Server #%d\n", i + 1);
        
        server_pids[i] = fork();
        if (server_pids[i] == 0) {
            // Child process - run server
            char server_id_str[16];
            snprintf(server_id_str, sizeof(server_id_str), "%d", i + 1);
            execl("./server", "server", server_id_str, NULL);
            perror("execl server");
            exit(1);
        } else if (server_pids[i] == -1) {
            perror("fork server");
            exit(1);
        }
    }
    
    // Give servers time to start
    sleep(1);
}

int main() {
    printf("[Watchdog]: Started\n");
    
    setup_signals();
    create_load_balancer();
    create_reverse_proxies();
    create_servers();
    
    // Main watchdog loop
    while (!should_exit) {
        pause(); // Wait for signals
    }
    
    return 0;
} 
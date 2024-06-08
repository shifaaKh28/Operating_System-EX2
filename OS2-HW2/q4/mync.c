#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <netdb.h>
#include <poll.h>
#include <ctype.h>

#define SIZE 3  // Define the size of the Tic-Tac-Toe board

// Function to execute a command
void executeCommand(char *args) {
    // Tokenize the arguments
    char *token = strtok(args, " ");
    if (token == NULL) {
        fprintf(stderr, "No arguments provided\n");
        exit(1);
    }

    // Initialize array to store arguments
    char **arguments = NULL;
    int n = 0;

    // Loop through tokens and store them in the array
    while (token != NULL) {
        arguments = realloc(arguments, (n + 1) * sizeof(char *));
        if (arguments == NULL) {
            exit(1);
        }
        arguments[n++] = token;
        token = strtok(NULL, " ");
    }

    // Add NULL terminator to the arguments array
    arguments = realloc(arguments, (n + 1) * sizeof(char *));
    if (arguments == NULL) {
        exit(1);
    }
    arguments[n] = NULL;

    // Fork a child process
    int pid = fork();
    if (pid < 0) {
        exit(1);
    }

    // Execute the command in the child process
    if (pid == 0) {
        execvp(arguments[0], arguments);
        perror("Error executing command");
        exit(1);
    } else {
        // Wait for the child process to finish
        wait(NULL);
        // Free the memory allocated for arguments
        free(arguments);
        fflush(stdout);
    }
}

// Signal handler for timeout
void handle_timeout(int signal) {
    exit(0);
}

// Function to close descriptors
void close_descriptors(int *descriptors) {
    if (descriptors[0] != STDIN_FILENO) {
        close(descriptors[0]);
    }
    if (descriptors[1] != STDOUT_FILENO) {
        close(descriptors[1]);
    }
}

// Function to set up a TCP server
void setup_TCPServer(int *descriptors, int port, char *b_flag) {
    // Create a TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating TCP socket");
        exit(EXIT_FAILURE);
    }
    printf("TCP socket has been created!\n");

    // Set socket options for reusing address
    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        perror("Error setting socket options");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the socket to the server address
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding TCP socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(sockfd, 1) < 0) {
        perror("Error listening for connections");
        exit(EXIT_FAILURE);
    }

    // Accept a client connection
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("Error accepting client connection");
        exit(EXIT_FAILURE);
    }

    // Update descriptors array with client file descriptor
    descriptors[0] = client_fd;

    // Duplicate client file descriptor for bidirectional communication
    if (b_flag != NULL) {
        descriptors[1] = client_fd;
    }
}

// Function to set up a TCP client
void setup_TCPClient(int *descriptors, char *ip, int port) {
    // Create a TCP socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Error creating socket");
        exit(1);
    }

    // Initialize server address structure
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Set socket options for reusing address
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("Error setting socket options");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Convert IP address to network format
    if (strcmp(ip, "localhost") == 0) {
        ip = "127.0.0.1";
    }
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid IP address");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error connecting to server");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Update descriptors array with socket file descriptor
    descriptors[1] = sock;

    printf("Successfully connected to %s:%d\n", ip, port);
    fflush(stdout);
}

// Function to set up a UDP server
void setup_UDPServer(int *descriptors, int port, int timeout) {
    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("UDP socket creation error");
        close_descriptors(descriptors);
        exit(1);
    }
    printf("UDP Socket created\n");

    // Set socket options for reusing address
    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("UDP setsockopt error");
        close_descriptors(descriptors);
        exit(1);
    }

    // Initialize server address structure
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the socket to the server address
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("UDP bind error");
        close_descriptors(descriptors);
        exit(1);
    }

    // Receive data from a client
    char buffer[1024];
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int numbytes = recvfrom(sockfd, buffer, sizeof

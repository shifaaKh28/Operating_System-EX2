#include <stdio.h>      // Standard Input Output functions
#include <stdlib.h>     // Standard library functions like memory allocation
#include <unistd.h>     // Provides access to the POSIX API
#include <sys/types.h>  // Defines data types used in system calls
#include <sys/socket.h> // Definitions for the socket API
#include <sys/un.h>     // Definitions for Unix domain sockets
#include <netinet/in.h> // Constants and structures for internet domain addresses
#include <arpa/inet.h>  // Functions for internet operations
#include <sys/wait.h>   // Macros related to process termination
#include <string.h>     // String handling functions
#include <getopt.h>     // Command-line option parsing
#include <errno.h>      // Error number definitions
#include <fcntl.h>      // File control options
#include <signal.h>     // Signal handling
#include <netdb.h>      // Network database operations
#include <poll.h>       // Polling for file descriptor events
#include <ctype.h>      // Character handling functions

#define SIZE 3  // Define the size of the Tic-Tac-Toe board

// Function to execute a command provided as a string of arguments
void executeCommand(char *args) {
    char *token = strtok(args, " "); // Tokenize the input string
    if (token == NULL) {
        fprintf(stderr, "No arguments provided\n");
        exit(1);
    }

    char **arguments = NULL; // Array of argument strings
    int n = 0; // Number of arguments

    // Tokenize the input string into individual arguments
    while (token != NULL) {
        arguments = realloc(arguments, (n + 1) * sizeof(char *)); // Reallocate memory for arguments array
        if (arguments == NULL) {
            exit(1);
        }
        arguments[n++] = token;
        token = strtok(NULL, " ");
    }

    arguments = realloc(arguments, (n + 1) * sizeof(char *)); // Null-terminate the arguments array
    if (arguments == NULL) {
        exit(1);
    }
    arguments[n] = NULL;

    int pid = fork(); // Create a new process
    if (pid < 0) {
        exit(1);
    }

    if (pid == 0) { // Child process
        execvp(arguments[0], arguments); // Execute the command
        perror("Error executing command"); // If execvp fails
        exit(1);
    } else { // Parent process
        wait(NULL); // Wait for the child process to finish
        free(arguments); // Free allocated memory
        fflush(stdout); // Flush the output buffer
    }
}

// Signal handler for timeout
void handle_timeout(int signal) {
    exit(0);
}

// Function to close file descriptors
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
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // Create a TCP socket
    if (sockfd < 0) {
        perror("Error creating TCP socket");
        exit(EXIT_FAILURE);
    }
    printf("TCP socket has been created!\n");

    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        perror("Error setting socket options");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr; // Server address structure
    server_addr.sin_family = AF_INET; // Address family
    server_addr.sin_port = htons(port); // Port number
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Bind to any local address

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding TCP socket");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 1) < 0) {
        perror("Error listening for connections");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in client_addr; // Client address structure
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len); // Accept a client connection
    if (client_fd < 0) {
        perror("Error accepting client connection");
        exit(EXIT_FAILURE);
    }

    descriptors[0] = client_fd; // Set the input descriptor to the client socket

    if (b_flag != NULL) {
        descriptors[1] = client_fd; // Set the output descriptor to the client socket if b_flag is set
    }
}

// Function to set up a TCP client
void setup_TCPClient(int *descriptors, char *ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0); // Create a TCP socket
    if (sock == -1) {
        perror("Error creating socket");
        exit(1);
    }

    printf("Setting up TCP client to connect to %s:%d\n", ip, port);
    fflush(stdout);

    struct sockaddr_in server_addr; // Server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // Address family
    server_addr.sin_port = htons(port); // Port number

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("Error setting socket options");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (strcmp(ip, "localhost") == 0) {
        ip = "127.0.0.1"; // Convert "localhost" to "127.0.0.1"
    }

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) { // Convert IP address to binary form
        perror("Invalid IP address");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error connecting to server");
        close(sock);
        exit(EXIT_FAILURE);
    }

    descriptors[1] = sock; // Set the output descriptor to the socket

    printf("Successfully connected to %s:%d\n", ip, port);
    fflush(stdout);
}

// Function to set up a UDP server
void setup_UDPServer(int *descriptors, int port, int timeout) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0); // Create a UDP socket
    if (sockfd == -1) {
        perror("UDP socket creation error");
        close_descriptors(descriptors);
        exit(1);
    }
    printf("UDP Socket created\n");

    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("UDP setsockopt error");
        close_descriptors(descriptors);
        exit(1);
    }

    struct sockaddr_in server_addr; // Server address structure
    server_addr.sin_family = AF_INET; // Address family
    server_addr.sin_port = htons(port); // Port number
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Bind to any local address

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("UDP bind error");
        close_descriptors(descriptors);
        exit(1);
    }

    char buffer[1024];
    struct sockaddr_in client_addr; // Client address structure
    socklen_t client_addr_len = sizeof(client_addr);
    int numbytes = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_addr_len); // Receive data from client
    if (numbytes == -1) {
        perror("UDP receive data error");
        close_descriptors(descriptors);
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1) {
        perror("UDP connect to client error");
        close_descriptors(descriptors);
        exit(1);
    }

    if (sendto(sockfd, "ACK", 3, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("UDP send ACK error");
        exit(1);
    }

    descriptors[0] = sockfd; // Set the input descriptor to the socket
    alarm(timeout); // Set an alarm for the timeout
}

// Function to set up a UDP client
void setup_UDPClient(int *descriptors, char *ip, int port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0); // Create a UDP socket
    if (sockfd == -1) {
        perror("UDP socket creation error");
        exit(1);
    }
    printf("UDP client\n");
    fflush(stdout);

    struct sockaddr_in server_addr; // Server address structure
    server_addr.sin_family = AF_INET; // Address family
    server_addr.sin_port = htons(port); // Port number

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) { // Convert IP address to binary form
        perror("Invalid server address");
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("UDP connect to server error");
        exit(1);
    }

    descriptors[1] = sockfd; // Set the output descriptor to the socket
}

// Function to set up a Unix domain socket datagram (UDS-D) server
void setup_UDSSDServer(int *descriptors, const char *path) {
    int sockfd = socket(AF_UNIX, SOCK_DGRAM, 0); // Create a Unix domain socket (datagram)
    if (sockfd == -1) {
        perror("Error creating Unix domain socket (datagram)");
        exit(1);
    }

    struct sockaddr_un server_addr; // Server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX; // Address family
    strncpy(server_addr.sun_path, path, sizeof(server_addr.sun_path) - 1); // Set the socket path

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding Unix domain socket (datagram)");
        close(sockfd);
        exit(1);
    }

    descriptors[0] = sockfd; // Set the input descriptor to the socket
}

// Function to set up a Unix domain socket datagram (UDS-D) client
void setup_UDSCDClient(int *descriptors, const char *path) {
    int sockfd = socket(AF_UNIX, SOCK_DGRAM, 0); // Create a Unix domain socket (datagram)
    if (sockfd == -1) {
        perror("Error creating Unix domain socket (datagram)");
        exit(1);
    }

    struct sockaddr_un server_addr; // Server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX; // Address family
    strncpy(server_addr.sun_path, path, sizeof(server_addr.sun_path) - 1); // Set the socket path

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error connecting to Unix domain socket (datagram)");
        close(sockfd);
        exit(1);
    }

    descriptors[1] = sockfd; // Set the output descriptor to the socket
}

// Function to set up a Unix domain socket stream (UDS-S) server
void setup_UDSSSServer(int *descriptors, const char *path) {
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0); // Create a Unix domain socket (stream)
    if (sockfd == -1) {
        perror("Error creating Unix domain socket (stream)");
        exit(1);
    }

    struct sockaddr_un server_addr; // Server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX; // Address family
    strncpy(server_addr.sun_path, path, sizeof(server_addr.sun_path) - 1); // Set the socket path

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding Unix domain socket (stream)");
        close(sockfd);
        exit(1);
    }

    if (listen(sockfd, 1) == -1) {
        perror("Error listening on Unix domain socket (stream)");
        close(sockfd);
        exit(1);
    }

    int client_fd = accept(sockfd, NULL, NULL); // Accept a client connection
    if (client_fd == -1) {
        perror("Error accepting connection on Unix domain socket (stream)");
        close(sockfd);
        exit(1);
    }

    descriptors[0] = client_fd; // Set the input descriptor to the client socket
}

// Function to set up a Unix domain socket stream (UDS-S) client
void setup_UDSCSClient(int *descriptors, const char *path) {
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0); // Create a Unix domain socket (stream)
    if (sockfd == -1) {
        perror("Error creating Unix domain socket (stream)");
        exit(1);
    }

    struct sockaddr_un server_addr; // Server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX; // Address family
    strncpy(server_addr.sun_path, path, sizeof(server_addr.sun_path) - 1); // Set the socket path

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error connecting to Unix domain socket (stream)");
        close(sockfd);
        exit(1);
    }

    descriptors[1] = sockfd; // Set the output descriptor to the socket
}

// Function to handle poll events
void handle_poll_event(struct pollfd *fds, int *descriptors) {
    if (fds[0].revents & POLLIN) {
        char buffer[1024];
        int bytes_read = read(fds[0].fd, buffer, sizeof(buffer)); // Read from input descriptor
        if (bytes_read == -1) {
            fprintf(stderr, "Error reading from input descriptor: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (bytes_read == 0) {
            return;
        }
        if (write(fds[1].fd, buffer, bytes_read) == -1) { // Write to output descriptor
            fprintf(stderr, "Error writing to output descriptor: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    if (fds[1].revents & POLLIN) {
        char buffer[1024];
        int bytes_read = read(fds[1].fd, buffer, sizeof(buffer)); // Read from output descriptor
        if (bytes_read == -1) {
            fprintf(stderr, "Error reading from output descriptor: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (bytes_read == 0) {
            return;
        }
        if (write(fds[3].fd, buffer, bytes_read) == -1) { // Write to stdout
            fprintf(stderr, "Error writing to stdout: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    if (fds[2].revents & POLLIN) {
        char buffer[1024];
        int bytes_read = read(fds[2].fd, buffer, sizeof(buffer)); // Read from stdin
        if (bytes_read == -1) {
            fprintf(stderr, "Error reading from stdin: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (bytes_read == 0) {
            return;
        }
        if (write(descriptors[1], buffer, bytes_read) == -1) { // Write to output descriptor
            fprintf(stderr, "Error writing to output descriptor: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int option;
    char *exec_command = NULL; // Command to execute
    char *input_type = NULL;   // Input type
    char *output_type = NULL;  // Output type
    char *timeout = NULL;      // Timeout value

    // Parse command-line options
    while ((option = getopt(argc, argv, "e:i:o:t:")) != -1) {
        switch (option) {
        case 'e':
            exec_command = optarg; // Set the command to execute
            break;
        case 'i':
            input_type = optarg; // Set the input type
            break;
        case 'o':
            output_type = optarg; // Set the output type
            break;
        case 't':
            timeout = optarg; // Set the timeout value
            break;
        default:
            fprintf(stderr, "Usage: %s <port>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (timeout != NULL) {
        signal(SIGALRM, handle_timeout); // Set the signal handler for timeout
        alarm(atoi(timeout)); // Set the alarm for timeout
    }

    int descriptors[2]; // File descriptors for input and output
    descriptors[0] = STDIN_FILENO; // Default input descriptor
    descriptors[1] = STDOUT_FILENO; // Default output descriptor

    // Setup input type
    if (input_type != NULL) {
        printf(" i = : %s\n", input_type);
        if (strncmp(input_type, "TCPS", 4) == 0) {
            input_type += 4;
            int port = atoi(input_type);
            setup_TCPServer(descriptors, port, NULL); // Setup TCP server
        } else if (strncmp(input_type, "UDPS", 4) == 0) {
            input_type += 4;
            int port = atoi(input_type);
            if (timeout != NULL) {
                setup_UDPServer(descriptors, port, atoi(timeout)); // Setup UDP server with timeout
            } else {
                setup_UDPServer(descriptors, port, 0); // Setup UDP server without timeout
            }
        } else if (strncmp(input_type, "UDSSD", 5) == 0) {
            input_type += 5;
            setup_UDSSDServer(descriptors, input_type); // Setup Unix domain socket datagram server
        } else if (strncmp(input_type, "UDSSS", 5) == 0) {
            input_type += 5;
            setup_UDSSSServer(descriptors, input_type); // Setup Unix domain socket stream server
        } else {
            fprintf(stderr, "Invalid server.\n");
            exit(1);
        }
    }

    // Setup output type
    if (output_type != NULL) {
        if (strncmp(output_type, "TCPC", 4) == 0) {
            output_type += 4;
            char *ip_server = strtok(output_type, ",");

            if (ip_server == NULL) {
                fprintf(stderr, "Invalid server IP\n");
                close_descriptors(descriptors);
                exit(1);
            }

            char *port_number = strtok(NULL, ",");
            if (port_number == NULL) {
                fprintf(stderr, "Invalid port\n");
                close_descriptors(descriptors);
                exit(1);
            }
            int port = atoi(port_number);
            setup_TCPClient(descriptors, ip_server, port); // Setup TCP client
        } else if (strncmp(output_type, "UDPC", 4) == 0) {
            output_type += 4;
            char *ip_server = strtok(output_type, ",");

            if (ip_server == NULL) {
                fprintf(stderr, "Invalid server IP\n");
                close_descriptors(descriptors);
                exit(1);
            }

            char *port_number = strtok(NULL, ",");
            if (port_number == NULL) {
                fprintf(stderr, "Invalid port\n");
                close_descriptors(descriptors);
                exit(1);
            }
            int port = atoi(port_number);
            setup_UDPClient(descriptors, ip_server, port); // Setup UDP client
        } else if (strncmp(output_type, "UDSCD", 5) == 0) {
            output_type += 5;
            setup_UDSCDClient(descriptors, output_type); // Setup Unix domain socket datagram client
        } else if (strncmp(output_type, "UDSCS", 5) == 0) {
            output_type += 5;
            setup_UDSCSClient(descriptors, output_type); // Setup Unix domain socket stream client
        } else {
            fprintf(stderr, "Invalid server.\n");
            close_descriptors(descriptors);
            exit(1);
        }
    }

    // Execute the command if provided
    if (exec_command != NULL) {
        if (descriptors[0] != STDIN_FILENO) {
            if (dup2(descriptors[0], STDIN_FILENO) == -1) {
                close(descriptors[0]);
                if (descriptors[1] != STDOUT_FILENO) {
                    close(descriptors[1]);
                }
                fprintf(stderr, "failed to duplicating input descriptor: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
        if (descriptors[1] != STDOUT_FILENO) {
            if (dup2(descriptors[1], STDOUT_FILENO) == -1) {
                close(descriptors[1]);
                if (descriptors[0] != STDIN_FILENO) {
                    close(descriptors[0]);
                }
                fprintf(stderr, "failed to duplicating output descriptor: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
        executeCommand(exec_command);
    } else {
        struct pollfd poll_file_descriptors[4]; // Poll file descriptors
        int num_file_descriptors = 4;

        poll_file_descriptors[0].fd = descriptors[0];
        poll_file_descriptors[0].events = POLLIN;

        poll_file_descriptors[1].fd = descriptors[1];
        poll_file_descriptors[1].events = POLLIN;

        poll_file_descriptors[2].fd = STDIN_FILENO;
        poll_file_descriptors[2].events = POLLIN;

        poll_file_descriptors[3].fd = STDOUT_FILENO;
        poll_file_descriptors[3].events = POLLIN;

        // Poll loop
        while (1) {
            int poll_result = poll(poll_file_descriptors, num_file_descriptors, -1);
            if (poll_result == -1) {
                fprintf(stderr, "Error polling: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            handle_poll_event(poll_file_descriptors, descriptors); // Handle poll events
        }
    }

    close(descriptors[0]); // Close input descriptor
    close(descriptors[1]); // Close output descriptor

    return 0; // Return success
}

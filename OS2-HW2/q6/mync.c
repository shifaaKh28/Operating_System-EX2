#include <stdio.h>  // Standard I/O library
#include <stdlib.h>  // Standard library for general functions
#include <unistd.h>  // Unix standard functions
#include <sys/types.h>  // Data types for system calls
#include <sys/socket.h>  // Sockets API
#include <sys/un.h>  // Unix domain sockets
#include <netinet/in.h>  // Internet domain address structures
#include <arpa/inet.h>  // Functions for IP address conversion
#include <sys/wait.h>  // Waiting for process termination
#include <string.h>  // String manipulation functions
#include <getopt.h>  // Command line option parsing
#include <errno.h>  // Error number definitions
#include <fcntl.h>  // File control options
#include <signal.h>  // Signal handling
#include <netdb.h>  // Network database operations
#include <poll.h>  // Polling for events on file descriptors
#include <ctype.h>  // Character type functions

#define SIZE 3  // Define the size of the Tic-Tac-Toe board

/**
 * @brief Executes a given command with its arguments.
 * 
 * @param args The command and its arguments as a single string.
 */
void executeCommand(char *args) {
    // Tokenize the input arguments string
    char *token = strtok(args, " ");
    if (token == NULL) {
        fprintf(stderr, "No arguments provided\n");
        exit(1);
    }

    // Initialize an array to hold the command and its arguments
    char **arguments = NULL;
    int n = 0;

    // Parse the tokenized arguments and store them in the array
    while (token != NULL) {
        arguments = realloc(arguments, (n + 1) * sizeof(char *));
        if (arguments == NULL) {
            exit(1);
        }
        arguments[n++] = token;
        token = strtok(NULL, " ");
    }

    // Add a NULL terminator to the arguments array
    arguments = realloc(arguments, (n + 1) * sizeof(char *));
    if (arguments == NULL) {
        exit(1);
    }
    arguments[n] = NULL;

    // Fork a new process to execute the command
    int pid = fork();
    if (pid < 0) {
        exit(1);
    }

    // Child process
    if (pid == 0) {
        // Execute the command
        execvp(arguments[0], arguments);
        // If execvp fails, print an error message and exit
        perror("Error executing command");
        exit(1);
    } else { // Parent process
        // Wait for the child process to finish
        wait(NULL);
        // Free memory allocated for the arguments array
        free(arguments);
        // Flush stdout to ensure all output is printed before returning
        fflush(stdout);
    }
}

/**
 * @brief Signal handler for the timeout.
 * 
 * @param signal The signal number.
 */
void handle_timeout(int signal) {
    // Terminate the process
    exit(0);
}

/**
 * @brief Closes the descriptors if they are not standard input/output.
 * 
 * @param descriptors An array containing the descriptors to close.
 */
void close_descriptors(int *descriptors) {
    // Close descriptors if they are not standard input/output
    if (descriptors[0] != STDIN_FILENO) {
        close(descriptors[0]);
    }
    if (descriptors[1] != STDOUT_FILENO) {
        close(descriptors[1]);
    }
}

/**
 * @brief Sets up a TCP server.
 * 
 * @param descriptors An array to store the file descriptors.
 * @param port The port number to bind to.
 * @param b_flag Optional flag for bidirectional communication.
 */
void setup_TCPServer(int *descriptors, int port, char *b_flag) {
    // Create a TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating TCP socket");
        exit(EXIT_FAILURE);
    }
    printf("TCP socket has been created!\n");

    // Allow the socket to be reused
    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        perror("Error setting socket options");
        exit(EXIT_FAILURE);
    }

    // Set up the server address structure
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the socket
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding TCP socket");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
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

    // Set the client file descriptor in the descriptors array
    descriptors[0] = client_fd;

    // If bidirectional communication is requested, set the second descriptor to the same client file descriptor
    if (b_flag != NULL) {
        descriptors[1] = client_fd;
    }
}

/**
 * @brief Sets up a TCP client.
 * 
 * @param descriptors An array to store the file descriptors.
 * @param ip The IP address to connect to.
 * @param port The port number to connect to.
 */
void setup_TCPClient(int *descriptors, char *ip, int port) {
    // Create a TCP socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Error creating socket");
        exit(1);
    }

    // Print message indicating TCP client setup
    printf("Setting up TCP client to connect to %s:%d\n", ip, port);
    fflush(stdout);

    // Set up server address structure
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Set socket option to allow address reuse
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("Error setting socket options");
        close(sock);
        exit(1);
    }

    // Convert "localhost" to the loopback address
    if (strcmp(ip, "localhost") == 0) {
        ip = "127.0.0.1";
    }

    // Convert IP address string to binary representation
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid IP address");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error connecting to server");
        close(sock);
        exit(1);
    }

    // Store the client socket descriptor in the descriptors array
    descriptors[1] = sock;

    // Print successful connection message
    printf("Successfully connected to %s:%d\n", ip, port);
    fflush(stdout);
}

/**
 * @brief Sets up a UDP server.
 * 
 * @param descriptors An array to store the file descriptors.
 * @param port The port number to bind to.
 * @param timeout The timeout value in seconds.
 */
void setup_UDPServer(int *descriptors, int port, int timeout) {
    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("UDP socket creation error");
        close_descriptors(descriptors);
        exit(1);
    }
    printf("UDP Socket created\n");

    // Enable address reuse for the socket
    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("UDP setsockopt error");
        close_descriptors(descriptors);
        exit(1);
    }

    // Set up server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind socket to server address
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("UDP bind error");
        close_descriptors(descriptors);
        exit(1);
    }

    // Receive data from client
    char buffer[1024];
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int numbytes = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_addr_len);
    if (numbytes == -1) {
        perror("UDP receive data error");
        close_descriptors(descriptors);
        exit(1);
    }

    // Connect to client
    if (connect(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1) {
        perror("UDP connect to client error");
        close_descriptors(descriptors);
        exit(1);
    }

    // Send ACK to client
    if (sendto(sockfd, "ACK", 3, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("UDP send ACK error");
        exit(1);
    }

    // Store the server socket descriptor in the descriptors array
    descriptors[0] = sockfd;
    
    // Set timeout using alarm signal
    alarm(timeout);
}

/**
 * @brief Sets up a UDP client.
 * 
 * @param descriptors An array to store the file descriptors.
 * @param ip The IP address to connect to.
 * @param port The port number to connect to.
 */
void setup_UDPClient(int *descriptors, char *ip, int port) {
    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("UDP socket creation error");
        exit(1);
    }
    printf("UDP client\n");
    fflush(stdout);

    // Set up server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid server address");
        exit(1);
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("UDP connect to server error");
        exit(1);
    }

    // Store the client socket descriptor in the descriptors array
    descriptors[1] = sockfd; 
}

/**
 * @brief Sets up a Unix domain socket datagram server.
 * 
 * @param descriptors An array to store the file descriptors.
 * @param path The path to bind the socket to.
 */
void setup_UDSSDServer(int *descriptors, const char *path) {
    // Create a Unix domain datagram socket
    int sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("Error creating Unix domain socket (datagram)");
        exit(1);
    }

    // Set up server address
    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, path, sizeof(server_addr.sun_path) - 1);

    // Remove the socket file if it already exists
    unlink(path);

    // Bind socket to server address
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding Unix domain socket (datagram)");
        close(sockfd);
        exit(1);
    }

    printf("Unix domain datagram server started on %s\n", path);
    descriptors[0] = sockfd;
}

/**
 * @brief Sets up a Unix domain socket datagram client.
 * 
 * @param descriptors An array to store the file descriptors.
 * @param path The path to connect the socket to.
 */
void setup_UDSCDClient(int *descriptors, const char *path) {
    // Create a Unix domain datagram socket
    int sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("Error creating Unix domain socket (datagram)");
        exit(1);
    }

    // Set up server address
    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, path, sizeof(server_addr.sun_path) - 1);

    printf("Connecting to Unix domain datagram server at %s\n", path);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error connecting to Unix domain socket (datagram)");
        close(sockfd);
        exit(1);
    }

    printf("Connected to Unix domain datagram server at %s\n", path);
    descriptors[1] = sockfd;
}

/**
 * @brief Sets up a Unix domain socket stream server.
 * 
 * @param descriptors An array to store the file descriptors.
 * @param path The path to bind the socket to.
 */
void setup_UDSSSServer(int *descriptors, const char *path) {
    // Create a Unix domain stream socket
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Error creating Unix domain socket (stream)");
        exit(1);
    }

    // Set up server address
    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, path, sizeof(server_addr.sun_path) - 1);

    // Remove the socket file if it already exists
    unlink(path);

    // Bind socket to server address
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding Unix domain socket (stream)");
        close(sockfd);
        exit(1);
    }

    // Listen for connections
    if (listen(sockfd, 1) == -1) {
        perror("Error listening on Unix domain socket (stream)");
        close(sockfd);
        exit(1);
    }

    printf("Unix domain stream server started on %s\n", path);

    // Accept a client connection
    int client_fd = accept(sockfd, NULL, NULL);
    if (client_fd == -1) {
        perror("Error accepting connection on Unix domain socket (stream)");
        close(sockfd);
        exit(1);
    }

    // Store the client socket descriptor in the descriptors array
    descriptors[0] = client_fd;
}

/**
 * @brief Sets up a Unix domain socket stream client.
 * 
 * @param descriptors An array to store the file descriptors.
 * @param path The path to connect the socket to.
 */
void setup_UDSCSClient(int *descriptors, const char *path) {
    // Create a Unix domain stream socket
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Error creating Unix domain socket (stream)");
        exit(1);
    }

    // Set up server address
    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, path, sizeof(server_addr.sun_path) - 1);

    printf("Connecting to Unix domain stream server at %s\n", path);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error connecting to Unix domain socket (stream)");
        close(sockfd);
        exit(1);
    }

    printf("Connected to Unix domain stream server at %s\n", path);
    descriptors[1] = sockfd;
}

/**
 * @brief Handles events for polling descriptors.
 * 
 * @param fds The poll file descriptors.
 * @param descriptors The descriptors to read/write.
 */
void handle_poll_event(struct pollfd *fds, int *descriptors) {
    if (fds[0].revents & POLLIN) {
        char buffer[1024];
        int bytes_read = read(fds[0].fd, buffer, sizeof(buffer));
        if (bytes_read == -1) {
            fprintf(stderr, "Error reading from input descriptor: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (bytes_read == 0) {
            return;
        }
        if (write(fds[1].fd, buffer, bytes_read) == -1) {
            fprintf(stderr, "Error writing to output descriptor: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    if (fds[1].revents & POLLIN) {
        char buffer[1024];
        int bytes_read = read(fds[1].fd, buffer, sizeof(buffer));
        if (bytes_read == -1) {
            fprintf(stderr, "Error reading from output descriptor: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (bytes_read == 0) {
            return;
        }
        if (write(fds[3].fd, buffer, bytes_read) == -1) {
            fprintf(stderr, "Error writing to stdout: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    if (fds[2].revents & POLLIN) {
        char buffer[1024];
        int bytes_read = read(fds[2].fd, buffer, sizeof(buffer));
        if (bytes_read == -1) {
            fprintf(stderr, "Error reading from stdin: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (bytes_read == 0) {
            return;
        }
        if (write(descriptors[1], buffer, bytes_read) == -1) {
            fprintf(stderr, "Error writing to output descriptor: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}
int main(int argc, char *argv[]) {
    // Check if the number of arguments is less than 2
    if (argc < 2) {
        // Print the usage message to stderr
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        // Exit the program with an error code
        exit(EXIT_FAILURE);
    }

    int option;  // Variable to store the current option parsed by getopt
    char *exec_command = NULL;  // Variable to store the command to execute
    char *input_type = NULL;  // Variable to store the input type
    char *output_type = NULL;  // Variable to store the output type
    char *timeout = NULL;  // Variable to store the timeout value

    // Parse command-line options using getopt
    while ((option = getopt(argc, argv, "e:i:o:t:")) != -1) {
        switch (option) {
            // If the option is 'e', store the argument in exec_command
            case 'e':
                exec_command = optarg;
                break;
            // If the option is 'i', store the argument in input_type
            case 'i':
                input_type = optarg;
                break;
            // If the option is 'o', store the argument in output_type
            case 'o':
                output_type = optarg;
                break;
            // If the option is 't', store the argument in timeout
            case 't':
                timeout = optarg;
                break;
            // If an unknown option is encountered, print the usage message and exit
            default:
                fprintf(stderr, "Usage: %s <port>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // If a timeout is specified, set up a signal handler for the alarm signal
    if (timeout != NULL) {
        signal(SIGALRM, handle_timeout);
        // Set the alarm to trigger after the specified number of seconds
        alarm(atoi(timeout));
    }

    int descriptors[2];  // Array to store file descriptors
    descriptors[0] = STDIN_FILENO;  // Default input is standard input
    descriptors[1] = STDOUT_FILENO; // Default output is standard output

    // If an input type is specified
    if (input_type != NULL) {
        // Print the input type
        printf(" i = : %s\n", input_type);
        // Check if the input type is TCP server
        if (strncmp(input_type, "TCPS", 4) == 0) {
            input_type += 4;  // Skip the "TCPS" prefix
            int port = atoi(input_type);  // Convert the port to an integer
            setup_TCPServer(descriptors, port, NULL);  // Set up a TCP server
        } 
        // Check if the input type is UDP server
        else if (strncmp(input_type, "UDPS", 4) == 0) {
            input_type += 4;  // Skip the "UDPS" prefix
            int port = atoi(input_type);  // Convert the port to an integer
            // Set up a UDP server with the specified timeout
            if (timeout != NULL) {
                setup_UDPServer(descriptors, port, atoi(timeout));
            } else {
                setup_UDPServer(descriptors, port, 0);
            }
        } 
        // Check if the input type is Unix domain socket datagram server
        else if (strncmp(input_type, "UDSSD", 5) == 0) {
            input_type += 5;  // Skip the "UDSSD" prefix
            setup_UDSSDServer(descriptors, input_type);  // Set up a Unix domain socket datagram server
        } 
        // Check if the input type is Unix domain socket stream server
        else if (strncmp(input_type, "UDSSS", 5) == 0) {
            input_type += 5;  // Skip the "UDSSS" prefix
            setup_UDSSSServer(descriptors, input_type);  // Set up a Unix domain socket stream server
        } 
        // If the input type is invalid, print an error message and exit
        else {
            fprintf(stderr, "Invalid input type: %s\n", input_type);
            exit(1);
        }
    }

    // If an output type is specified
    if (output_type != NULL) {
        // Print the output type
        printf(" o = : %s\n", output_type);
        // Check if the output type is TCP client
        if (strncmp(output_type, "TCPC", 4) == 0) {
            output_type += 4;  // Skip the "TCPC" prefix
            char *ip_server = strtok(output_type, ",");  // Extract the IP address
            if (ip_server == NULL) {
                fprintf(stderr, "Invalid server IP\n");
                close_descriptors(descriptors);
                exit(1);
            }
            char *port_number = strtok(NULL, ",");  // Extract the port number
            if (port_number == NULL) {
                fprintf(stderr, "Invalid port\n");
                close_descriptors(descriptors);
                exit(1);
            }
            int port = atoi(port_number);  // Convert the port to an integer
            setup_TCPClient(descriptors, ip_server, port);  // Set up a TCP client
        } 
        // Check if the output type is UDP client
        else if (strncmp(output_type, "UDPC", 4) == 0) {
            output_type += 4;  // Skip the "UDPC" prefix
            char *ip_server = strtok(output_type, ",");  // Extract the IP address
            if (ip_server == NULL) {
                fprintf(stderr, "Invalid server IP\n");
                close_descriptors(descriptors);
                exit(1);
            }
            char *port_number = strtok(NULL, ",");  // Extract the port number
            if (port_number == NULL) {
                fprintf(stderr, "Invalid port\n");
                close_descriptors(descriptors);
                exit(1);
            }
            int port = atoi(port_number);  // Convert the port to an integer
            setup_UDPClient(descriptors, ip_server, port);  // Set up a UDP client
        } 
        // Check if the output type is Unix domain socket datagram client
        else if (strncmp(output_type, "UDSCD", 5) == 0) {
            output_type += 5;  // Skip the "UDSCD" prefix
            setup_UDSCDClient(descriptors, output_type);  // Set up a Unix domain socket datagram client
        } 
        // Check if the output type is Unix domain socket stream client
        else if (strncmp(output_type, "UDSCS", 5) == 0) {
            output_type += 5;  // Skip the "UDSCS" prefix
            setup_UDSCSClient(descriptors, output_type);  // Set up a Unix domain socket stream client
        } 
        // Check if the output type is TCP server
        else if (strncmp(output_type, "TCPS", 4) == 0) {
            output_type += 4;  // Skip the "TCPS" prefix
            int port = atoi(output_type);  // Convert the port to an integer
            setup_TCPServer(descriptors, port, NULL);  // Set up a TCP server
        } 
        // Check if the output type is UDP server
        else if (strncmp(output_type, "UDPS", 4) == 0) {
            output_type += 4;  // Skip the "UDPS" prefix
            int port = atoi(output_type);  // Convert the port to an integer
            // Set up a UDP server with the specified timeout
            if (timeout != NULL) {
                setup_UDPServer(descriptors, port, atoi(timeout));
            } else {
                setup_UDPServer(descriptors, port, 0);
            }
        } 
        // Check if the output type is Unix domain socket stream server
        else if (strncmp(output_type, "UDSSS", 5) == 0) {
            output_type += 5;  // Skip the "UDSSS" prefix
            setup_UDSSSServer(descriptors, output_type);  // Set up a Unix domain socket stream server
            descriptors[1] = descriptors[0];  // Set descriptors[1] to the socket
            descriptors[0] = STDIN_FILENO;  // Set descriptors[0] to standard input
        } 
        // Check if the output type is Unix domain socket datagram server
        else if (strncmp(output_type, "UDSSD", 5) == 0) {
            output_type += 5;  // Skip the "UDSSD" prefix
            setup_UDSSDServer(descriptors, output_type);  // Set up a Unix domain socket datagram server
            descriptors[1] = descriptors[0];  // Set descriptors[1] to the socket
            descriptors[0] = STDIN_FILENO;  // Set descriptors[0] to standard input
        } 
        // If the output type is invalid, print an error message and exit
        else {
            fprintf(stderr, "Invalid output type: %s\n", output_type);
            close_descriptors(descriptors);
            exit(1);
        }
    }

    // If an execution command is specified
    if (exec_command != NULL) {
        // Redirect input descriptor to standard input if necessary
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
        // Redirect output descriptor to standard output if necessary
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
        // Execute the command
        executeCommand(exec_command);
    } else {  // If no execution command is specified, handle polling events
        struct pollfd poll_file_descriptors[4];  // Array to store poll file descriptors
        int num_file_descriptors = 4;  // Number of file descriptors to poll

        // Set up poll descriptors
        poll_file_descriptors[0].fd = descriptors[0];
        poll_file_descriptors[0].events = POLLIN;

        poll_file_descriptors[1].fd = descriptors[1];
        poll_file_descriptors[1].events = POLLIN;

        poll_file_descriptors[2].fd = STDIN_FILENO;
        poll_file_descriptors[2].events = POLLIN;

        poll_file_descriptors[3].fd = STDOUT_FILENO;
        poll_file_descriptors[3].events = POLLIN;

        // Polling loop
        while (1) {
            int poll_result = poll(poll_file_descriptors, num_file_descriptors, -1);
            if (poll_result == -1) {
                fprintf(stderr, "Error polling: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            // Handle poll events
            handle_poll_event(poll_file_descriptors, descriptors);
        }
    }

    // Close descriptors before exiting
    close(descriptors[0]);
    close(descriptors[1]);

    return 0;  // Return success
}

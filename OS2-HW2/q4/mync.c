#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
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

void executeCommand(char *args)
{
    // Tokenize the input arguments string
    char *token = strtok(args, " ");
    if (token == NULL)
    {
        fprintf(stderr, "No arguments provided\n");
        exit(1);
    }

    // Initialize an array to hold the command and its arguments
    char **arguments = NULL;
    int n = 0;

    // Parse the tokenized arguments and store them in the array
    while (token != NULL)
    {
        arguments = realloc(arguments, (n + 1) * sizeof(char *));
        if (arguments == NULL)
        {
            exit(1);
        }
        arguments[n++] = token;
        token = strtok(NULL, " ");
    }

    // Add a NULL terminator to the arguments array
    arguments = realloc(arguments, (n + 1) * sizeof(char *));
    if (arguments == NULL)
    {
        exit(1);
    }
    arguments[n] = NULL;

    // Fork a new process to execute the command
    int pid = fork();
    if (pid < 0)
    {
        exit(1);
    }

    // Child process
    if (pid == 0)
    {
        // Execute the command
        execvp(arguments[0], arguments);

        // If execvp fails, print an error message and exit
        perror("Error executing command");
        exit(1);
    }
    // Parent process
    else
    {
        // Wait for the child process to finish
        wait(NULL);

        // Free memory allocated for the arguments array
        free(arguments);

        // Flush stdout to ensure all output is printed before returning
        fflush(stdout);
    }
}

void handle_timeout(int signal)
{
    // Terminate the process
    exit(0);
}

void close_descriptors(int *descriptors)
{
    // Close descriptors if they are not standard input/output
    if (descriptors[0] != STDIN_FILENO)
    {
        close(descriptors[0]);
    }
    if (descriptors[1] != STDOUT_FILENO)
    {
        close(descriptors[1]);
    }
}
void setup_TCPServer(int *descriptors, int port, char *b_flag)
{
    // Create a TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Error creating TCP socket");
        exit(EXIT_FAILURE);
    }
    printf("TCP socket has been created!\n");

    // Allow the socket to be reused
    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
    {
        perror("Error setting socket options");
        exit(EXIT_FAILURE);
    }

    // Set up the server address structure
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Listen to any address

    // Bind the socket
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error binding TCP socket");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(sockfd, 1) < 0)
    {
        perror("Error listening for connections");
        exit(EXIT_FAILURE);
    }

    // Accept a client connection
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0)
    {
        perror("Error accepting client connection");
        exit(EXIT_FAILURE);
    }

    // Set the client file descriptor in the descriptors array
    descriptors[0] = client_fd;

    // If bidirectional communication is requested, set the second descriptor to the same client file descriptor
    if (b_flag != NULL)
    {
        descriptors[1] = client_fd;
    }
}

// Function to set up a TCP client
void setup_TCPClient(int *descriptors, char *ip, int port)
{
    // Create a TCP socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
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
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    {
        perror("Error setting socket options");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Convert "localhost" to the loopback address
    if (strcmp(ip, "localhost") == 0)
    {
        ip = "127.0.0.1";
    }

    // Convert IP address string to binary representation
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0)
    {
        perror("Invalid IP address");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Error connecting to server");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Store the client socket descriptor in the descriptors array
    descriptors[1] = sock;

    // Print successful connection message
    printf("Successfully connected to %s:%d\n", ip, port);
    fflush(stdout);
}

void setup_UDPServer(int *descriptors, int port, int timeout)
{
    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        perror("UDP socket creation error");
        close_descriptors(descriptors);
        exit(1);
    }
    printf("UDP Socket created\n");

    // Enable address reuse for the socket
    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
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
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("UDP bind error");
        close_descriptors(descriptors);
        exit(1);
    }

    // Receive data from client
    char buffer[1024];
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int numbytes = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_addr_len);
    if (numbytes == -1)
    {
        perror("UDP receive data error");
        close_descriptors(descriptors);
        exit(1);
    }

    // Connect to client
    if (connect(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1)
    {
        perror("UDP connect to client error");
        close_descriptors(descriptors);
        exit(1);
    }

    // Send ACK to client
    if (sendto(sockfd, "ACK", 3, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("UDP send ACK error");
        exit(1);
    }

    // Store the server socket descriptor in the descriptors array
    descriptors[0] = sockfd;
    
    // Set timeout using alarm signal
    alarm(timeout);
}

void setup_UDPClient(int *descriptors, char *ip, int port)
{
    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
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
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0)
    {
        perror("Invalid server address");
        exit(1);
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("UDP connect to server error");
        exit(1);
    }

    // Store the client socket descriptor in the descriptors array
    descriptors[1] = sockfd; 
}


void handle_poll_event(struct pollfd *fds, int *descriptors) {
    // Function to handle poll events
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
    if (argc < 2) {
        // Ensure there are enough arguments
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int option;
    char *exec_command = NULL;
    char *input_type = NULL;
    char *output_type = NULL;
    char *timeout = NULL;

    // Parse command-line options
    while ((option = getopt(argc, argv, "e:i:o:t:")) != -1) {
        switch (option) {
        case 'e':
            exec_command = optarg;
            break;
        case 'i':
            input_type = optarg;
            break;
        case 'o':
            output_type = optarg;
            break;
        case 't':
            timeout = optarg;
            break;
        default:
            fprintf(stderr, "Usage: %s <port>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // Set a timeout if specified
    if (timeout != NULL) {
        signal(SIGALRM, handle_timeout);
        alarm(atoi(timeout));
    }

    int descriptors[2];
    descriptors[0] = STDIN_FILENO;  // Default input is standard input
    descriptors[1] = STDOUT_FILENO; // Default output is standard output

    // Set up input type
    if (input_type != NULL) {
        printf(" i = : %s\n", input_type);
        if (strncmp(input_type, "TCPS", 4) == 0) {
            input_type += 4;
            int port = atoi(input_type);
            setup_TCPServer(descriptors, port, NULL);
        } else if (strncmp(input_type, "UDPS", 4) == 0) {
            input_type += 4;
            int port = atoi(input_type);
            if (timeout != NULL) {
                setup_UDPServer(descriptors, port, atoi(timeout));
            } else {
                setup_UDPServer(descriptors, port, 0);
            }
        } else {
            fprintf(stderr, "Invalid server.\n");
            exit(1);
        }
    }

    // Set up output type
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
            setup_TCPClient(descriptors, ip_server, port);
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
            setup_UDPClient(descriptors, ip_server, port);
        } else {
            fprintf(stderr, "Invalid server.\n");
            close_descriptors(descriptors);
            exit(1);
        }
    }

    // Execute the command if specified
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
                fprintf(stderr, "faialed to duplicating output descriptor: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
        executeCommand(exec_command);
   } else {
    struct pollfd poll_file_descriptors[4];
    int num_file_descriptors = 4;

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

return 0;
}
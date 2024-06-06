#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

/**
 * @brief Sets up a TCP server on the specified port and waits for incoming connections.
 * 
 * @param port The port number to listen on.
 * @return int The file descriptor of the socket for the accepted connection.
 */
int setup_server(int port) {
    printf("Setting up server on port %d\n", port);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // Create a socket
    if (sockfd < 0) {
        perror("Error opening socket"); // Print error message if socket creation fails
        exit(1); // Exit the program
    }

    // Set SO_REUSEADDR option
    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("Error setting socket option"); // Print error message if setting socket option fails
        close(sockfd); // Close the socket
        exit(1); // Exit the program
    }

    struct sockaddr_in serv_addr; // Structure for server address
    memset(&serv_addr, 0, sizeof(serv_addr)); // Clear the structure
    serv_addr.sin_family = AF_INET; // Set address family to IPv4
    serv_addr.sin_addr.s_addr = INADDR_ANY; // Accept connections on any interface
    serv_addr.sin_port = htons(port); // Set the port number

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error on binding"); // Print error message if binding fails
        close(sockfd); // Close the socket
        exit(1); // Exit the program
    }

    printf("Socket bound to port %d\n", port);
    listen(sockfd, 5); // Listen for connections
    printf("Server is listening\n");

    struct sockaddr_in cli_addr; // Structure for client address
    socklen_t clilen = sizeof(cli_addr); // Length of client address
    int newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen); // Accept connection
    if (newsockfd < 0) {
        perror("Error on accept"); // Print error message if accept fails
        close(sockfd); // Close the socket
        exit(1); // Exit the program
    }

    printf("Connection accepted\n");
    return newsockfd; // Return the new socket file descriptor
}

/**
 * @brief Sets up a TCP client connection to the specified host and port.
 * 
 * @param hostname The hostname or IP address of the server.
 * @param port The port number of the server.
 * @return int The file descriptor of the client socket.
 */
int setup_client(const char *hostname, int port) {
    printf("Setting up client for host %s on port %d\n", hostname, port);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // Create a socket
    if (sockfd < 0) {
        perror("Error opening socket"); // Print error message if socket creation fails
        exit(1); // Exit the program
    }

    struct sockaddr_in serv_addr; // Structure for server address
    struct hostent *server = gethostbyname(hostname); // Get host by name
    if (server == NULL) {
        fprintf(stderr, "Error, no such host\n"); // Print error message if host not found
        close(sockfd); // Close the socket
        exit(1); // Exit the program
    }
    memset(&serv_addr, 0, sizeof(serv_addr)); // Clear the structure
    serv_addr.sin_family = AF_INET; // Set address family to IPv4
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length); // Copy server address
    serv_addr.sin_port = htons(port); // Set the port number

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error connecting"); // Print error message if connection fails
        close(sockfd); // Close the socket
        exit(1); // Exit the program
    }

    printf("Client setup complete\n");
    return sockfd; // Return the socket file descriptor
}

/**
 * @brief Redirects input and output streams of a command to specified file descriptors.
 * 
 * @param input_fd The file descriptor for input.
 * @param output_fd The file descriptor for output.
 * @param command The command to execute.
 */
void redirect_io(int input_fd, int output_fd, char *command) {
    char *cmd_args[256]; // Array to store command and arguments
    int arg_count = 0; // Argument count
    char *token = strtok(command, " "); // Tokenize the command string
    while (token != NULL) {
        cmd_args[arg_count++] = token; // Store each token in the array
        token = strtok(NULL, " "); // Get the next token
    }
    cmd_args[arg_count] = NULL; // Null-terminate the array

    char exec_command[256]; // Buffer for executable command
    if (strchr(cmd_args[0], '/') == NULL) {
        snprintf(exec_command, sizeof(exec_command), "./%s", cmd_args[0]); // Create the full path for the executable
        cmd_args[0] = exec_command; // Update the command with the full path
    }

    pid_t pid = fork(); // Fork a child process
    if (pid < 0) {
        perror("Error"); // Print error message if fork fails
        exit(1); // Exit the program
    } else if (pid == 0) {
        // Child process
        if (dup2(input_fd, STDIN_FILENO) == -1) {
            perror("dup2 input_fd failed"); // Print error message if input redirection fails
            exit(1); // Exit the program
        }
        if (dup2(output_fd, STDOUT_FILENO) == -1) {
            perror("dup2 output_fd failed"); // Print error message if output redirection fails
            exit(1); // Exit the program
        }
        execvp(cmd_args[0], cmd_args); // Execute the command
        perror("exec failed"); // Print error message if exec fails
        exit(1); // Exit the program
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0); // Wait for child process to complete
        if (WIFEXITED(status)) {
            printf("Child exited with status %d\n", WEXITSTATUS(status)); // Print exit status of child process
        } else if (WIFSIGNALED(status)) {
            printf("Child killed by signal %d\n", WTERMSIG(status)); // Print signal that killed the child process
        }
        close(input_fd); // Close input file descriptor
        close(output_fd); // Close output file descriptor
    }
}

/**
 * @brief Handles input and output streams
 * 
 * @param input_fd The file descriptor for input.
 * @param output_fd The file descriptor for output.
 */
void handle_io(int input_fd, int output_fd) {
    char buffer[256]; // Buffer to store data read from input
    ssize_t n; // Number of bytes read
    // Read data from input and write to output until end of file is reached
    while ((n = read(input_fd, buffer, sizeof(buffer))) > 0) {
        if (write(output_fd, buffer, n) != n) {
            perror("write failed"); // Print error message if write fails
            exit(1); // Exit the program
        }
    }
    // Check for read errors
    if (n < 0) {
        perror("read failed"); // Print error message if read fails
        exit(1); // Exit the program
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [-e command] -i input_mode [-o output_mode] [-b bidirectional_mode]\n", argv[0]);
        exit(1); // Print usage message and exit if arguments are insufficient
    }

    int input_fd = STDIN_FILENO; // Default input file descriptor
    int output_fd = STDOUT_FILENO; // Default output file descriptor
    char *command = NULL; // Command to execute
    int bidirectional = 0; // Flag to indicate bidirectional communication

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0) { // Check for input mode option
            i++;
            if (strncmp(argv[i], "TCPS", 4) == 0) { // Check for TCP server mode
                int port = atoi(argv[i] + 4); // Extract port number
                input_fd = setup_server(port); // Set up server and get input file descriptor
            } else {
                fprintf(stderr, "Invalid input mode\n"); // Print error for invalid input mode
                exit(1); // Exit the program
            }
        } else if (strcmp(argv[i], "-o") == 0) { // Check for output mode option
            i++;
            if (strncmp(argv[i], "TCPC", 4) == 0) { // Check for TCP client mode
                char *input = argv[i] + 4; // Extract input string
                char *hostname = strtok(input, ","); // Extract hostname
                if (hostname == NULL) {
                    fprintf(stderr, "Invalid output mode, missing hostname\n"); // Print error for missing hostname
                    exit(1); // Exit the program
                }
                char *port_str = strtok(NULL, ","); // Extract port string
                if (port_str == NULL) {
                    fprintf(stderr, "Invalid output mode, missing port\n"); // Print error for missing port
                    exit(1); // Exit the program
                }
                int port = atoi(port_str); // Convert port string to integer
                output_fd = setup_client(hostname, port); // Set up client and get output file descriptor
            } else {
                fprintf(stderr, "Invalid output mode\n"); // Print error for invalid output mode
                exit(1); // Exit the program
            }
        } else if (strcmp(argv[i], "-b") == 0) { // Check for bidirectional mode option
            i++;
            if (strncmp(argv[i], "TCPS", 4) == 0) { // Check for bidirectional TCP server mode
                int port = atoi(argv[i] + 4); // Extract port number
                input_fd = setup_server(port); // Set up server and get input file descriptor
                output_fd = input_fd; // Set output file descriptor to input file descriptor
                bidirectional = 1; // Set bidirectional flag
            } else {
                fprintf(stderr, "Invalid bidirectional mode\n"); // Print error for invalid bidirectional mode
                exit(1); // Exit the program
            }
        } else if (strcmp(argv[i], "-e") == 0) { // Check for command option
            i++;
            command = argv[i]; // Set command to execute
        }
    }

    if (command != NULL) { // If command is specified
        printf("Executing command: %s\n", command); // Print the command
        if (bidirectional) {
            redirect_io(input_fd, input_fd, command); // Redirect IO for bidirectional communication
        } else {
            redirect_io(input_fd, output_fd, command); // Redirect IO for unidirectional communication
        }
    } else {
        handle_io(input_fd, output_fd); // Handle IO for default input/output
    }

    return 0; // Return success
}

#include <stdio.h>      // Standard I/O functions
#include <stdlib.h>     // Standard library functions
#include <unistd.h>     // UNIX standard function definitions
#include <sys/types.h>  // System data types
#include <sys/wait.h>   // Waiting for process termination
#include <string.h>     // String manipulation functions
#include <sys/socket.h> // Socket programming functions
#include <netinet/in.h> // Internet address family
#include <netdb.h>      // Network database operations
#include <arpa/inet.h>  // Definitions for internet operations

// Function to receive data from client
void receive_data(int sockfd)
{
    char buffer[256]; // Buffer to store received data
    int n;            // Number of bytes read

    while (1)
    {
        // Read data from the client
        n = read(sockfd, buffer, sizeof(buffer));
        if (n < 0)
        {
            perror("Error reading from socket"); // Print error message if read fails
            close(sockfd);                      // Close the socket
            exit(1);                            // Exit the program
        }
        else if (n == 0)
        {
            printf("Client disconnected\n");
            break; // Exit the loop if client disconnects
        }

        // Print the received data
        printf("Received message from client: %.*s\n", n, buffer);

    }
}

// Function to set up a TCP server
int setup_server(int port)
{
    printf("Setting up server on port %d\n", port);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // Create a socket
    if (sockfd < 0)
    {
        perror("Error opening socket"); // Print error message if socket creation fails
        exit(1);                        // Exit the program
    }

    // Set SO_REUSEADDR option
    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        perror("Error setting socket option"); // Print error message if setting socket option fails
        close(sockfd);                         // Close the socket
        exit(1);                               // Exit the program
    }

    struct sockaddr_in serv_addr;             // Structure for server address
    memset(&serv_addr, 0, sizeof(serv_addr)); // Clear the structure
    serv_addr.sin_family = AF_INET;           // Set address family to IPv4
    serv_addr.sin_addr.s_addr = INADDR_ANY;   // Accept connections on any interface
    serv_addr.sin_port = htons(port);         // Set the port number

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Error on binding"); // Print error message if binding fails
        close(sockfd);              // Close the socket
        exit(1);                    // Exit the program
    }

    printf("Socket bound to port %d\n", port);

    listen(sockfd, 5); // Listen for connections
    printf("Server is listening\n");

    struct sockaddr_in cli_addr;                                           // Structure for client address
    socklen_t clilen = sizeof(cli_addr);                                   // Length of client address
    int newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen); // Accept connection
    if (newsockfd < 0)
    {
        perror("Error on accept"); // Print error message if accept fails
        close(sockfd);             // Close the socket
        exit(1);                   // Exit the program
    }

    printf("Connection accepted\n");

    // Receive data from the client
    receive_data(newsockfd);

    return newsockfd; // Return the new socket file descriptor
}


// Function to set up a TCP client
int setup_client(const char *hostname, int port)
{
    printf("Setting up client for host %s on port %d\n", hostname, port);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // Create a socket
    if (sockfd < 0)
    {
        perror("Error opening socket"); // Print error message if socket creation fails
        exit(1);                        // Exit the program
    }

    struct sockaddr_in serv_addr;                     // Structure for server address
    struct hostent *server = gethostbyname(hostname); // Get host by name
    if (server == NULL)
    {
        fprintf(stderr, "Error, no such host\n"); // Print error message if host not found
        close(sockfd);                            // Close the socket
        exit(1);                                  // Exit the program
    }
    memset(&serv_addr, 0, sizeof(serv_addr));                                     // Clear the structure
    serv_addr.sin_family = AF_INET;                                               // Set address family to IPv4
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length); // Copy server address
    serv_addr.sin_port = htons(port);                                             // Set the port number

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Error connecting"); // Print error message if connection fails
        close(sockfd);              // Close the socket
        exit(1);                    // Exit the program
    }

    printf("Client setup complete\n");
    return sockfd; // Return the socket file descriptor
}

// Function to handle input and output redirection for each move
void redirect_io(int input_fd, int output_fd, char *command)
{
    char buffer[1024]; // Buffer for reading input

    // Infinite loop to continuously read input
    while (1)
    {
        ssize_t bytes_read;

        // Read input from the specified input file descriptor
        if (input_fd == STDIN_FILENO)
        {
            // Read input from standard input
            bytes_read = read(input_fd, buffer, sizeof(buffer));
        }
        else
        {
            // Read input from TCP connection
            bytes_read = recv(input_fd, buffer, sizeof(buffer), 0);
        }

        if (bytes_read < 0)
        {
            perror("Error reading input");
            exit(1);
        }
        else if (bytes_read == 0)
        {
            // End of input
            break;
        }
        else
        {
            // Write input to the specified output file descriptor
            if (output_fd == STDOUT_FILENO)
            {
                // Write output to standard output
                if (write(output_fd, buffer, bytes_read) < 0)
                {
                    perror("Error writing output");
                    exit(1);
                }
            }
            else
            {
                // Write output to TCP connection
                if (send(output_fd, buffer, bytes_read, 0) < 0)
                {
                    perror("Error sending output");
                    exit(1);
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s -i input_mode [-o output_mode] [-b bidirectional_mode]\n", argv[0]);
        exit(1); // Print usage message and exit if arguments are insufficient
    }

    int input_fd = STDIN_FILENO;   // Default input file descriptor
    int output_fd = STDOUT_FILENO; // Default output file descriptor
    char *command = NULL;          // Command to execute
    int bidirectional = 0;         // Flag to indicate bidirectional communication

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-i") == 0)
        { // Check for input mode option
            i++;
            if (strncmp(argv[i], "TCPS", 4) == 0)
            {                                  // Check for TCP server mode
                int port = atoi(argv[i] + 4);  // Extract port number
                input_fd = setup_server(port); // Set up server and get input file descriptor
            }
            else
            {
                fprintf(stderr, "Invalid input mode\n"); // Print error for invalid input mode
                exit(1);                                 // Exit the program
            }
        }
        else if (strcmp(argv[i], "-o") == 0)
        { // Check for output mode option
            i++;
            if (strncmp(argv[i], "TCPC", 4) == 0)
            {                                         // Check for TCP client mode
                char *input = argv[i] + 4;            // Extract input string
                char *localhost = strtok(input, ","); // Extract local host
                if (localhost == NULL)
                {
                    fprintf(stderr, "Invalid output mode, missing localhost\n"); // Print error for missing localhost
                    exit(1);                                                     // Exit the program
                }
                char *port_str = strtok(NULL, ","); // Extract port string
                if (port_str == NULL)
                {
                    fprintf(stderr, "Invalid output mode, missing port\n"); // Print error for missing port
                    exit(1);                                                // Exit the program
                }
                int port = atoi(port_str);                 // Convert port string to integer
                output_fd = setup_client(localhost, port); // Set up client and get output file descriptor
            }
            else
            {
                fprintf(stderr, "Invalid output mode\n"); // Print error for invalid output mode
                exit(1);                                  // Exit the program
            }
        }
        else if (strcmp(argv[i], "-b") == 0)
        { // Check for bidirectional mode option
            i++;
            if (strncmp(argv[i], "TCPS", 4) == 0)
            {                                  // Check for bidirectional TCP server mode
                int port = atoi(argv[i] + 4);  // Extract port number
                input_fd = setup_server(port); // Set up server and get input file descriptor
                output_fd = input_fd;          // Set output file descriptor to input file descriptor
                bidirectional = 1;             // Set bidirectional flag
            }
            else
            {
                fprintf(stderr, "Invalid bidirectional mode\n"); // Print error for invalid bidirectional mode
                exit(1);                                         // Exit the program
            }
        }
    }

    char buffer[1024]; // Buffer for reading input

    while (1)
    {
        // Read input from standard input
        int bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
        if (bytes_read == -1)
        {
            perror("Error reading from standard input");
            exit(EXIT_FAILURE);
        }
        if (bytes_read == 0)
        {
            break; // End of file
        }

        // Write output to standard output
        if (write(STDOUT_FILENO, buffer, bytes_read) == -1)
        {
            perror("Error writing to standard output");
            exit(EXIT_FAILURE);
        }
    }

    return 0; // Return success
}

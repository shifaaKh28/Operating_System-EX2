#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    if (argc < 3 || strcmp(argv[1], "-e") != 0) {
        fprintf(stderr, "Usage: %s -e \"command\"\n", argv[0]);
        return 1;
    }

    // Extract the command from the arguments
    char *command = argv[2];

    // Fork a child process
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) { // Child process
        // Parse the command string into a program and arguments
        char *args[100];
        char *token = strtok(command, " ");
        int i = 0;
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL; // Null-terminate the array

        // If the command contains a relative path or absolute path
        if (strchr(args[0], '/') != NULL) {
            if (execv(args[0], args) == -1) {
                perror("execv");
                exit(1);
            }
        } else {
            if (execvp(args[0], args) == -1) {
                perror("execvp");
                exit(1);
            }
        }
    } else { // Parent process
        // Wait for the child process to complete
        int status;
        waitpid(pid, &status, 0);
    }

    return 0;
}
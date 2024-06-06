#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define SIZE 3  // Define the size of the Tic-Tac-Toe board

/**
 * @brief Initializes the Tic-Tac-Toe board with empty spaces.
 * 
 * @param board The Tic-Tac-Toe board.
 */
void initializeBoard(char board[SIZE][SIZE]) {
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            board[i][j] = ' ';  // Set each cell to an empty space
        }
    }
}

/**
 * @brief Displays the current state of the Tic-Tac-Toe board.
 * 
 * @param board The Tic-Tac-Toe board.
 */
void displayBoard(char board[SIZE][SIZE]) {
    printf("-------------\n");
    fflush(stdout);  // Flush after printing the board border
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            printf("| %c ", board[i][j]);
            fflush(stdout);  // Flush after printing each cell
        }
        printf("|\n");
        fflush(stdout);  // Flush after printing row end
        printf("-------------\n");
        fflush(stdout);  // Flush after printing row border
    }
}

/**
 * @brief Validates the strategy string provided by the AI.
 * 
 * @param strategy The strategy string.
 * @return int Returns 1 if the strategy is valid, 0 otherwise.
 */
int validateStrategy(const char *strategy) {
    if (strlen(strategy) != 9) {
        return 0;  // Strategy must be exactly 9 characters long
    }

    int digitCount[10] = {0};  // Array to count occurrences of each digit

    for (int i = 0; i < 9; i++) {
        if (!isdigit(strategy[i])) {
            return 0;  // Each character must be a digit
        }

        int digit = strategy[i] - '0';  // Convert the character to an integer

        if (digit < 1 || digit > 9) {
            return 0;  // Digits must be between 1 and 9
        }

        digitCount[digit]++;  // Increment the count of the digit
    }

    for (int i = 1; i <= 9; i++) {
        if (digitCount[i] != 1) {
            return 0;  // Each digit must appear exactly once
        }
    }

    return 1;  // Strategy is valid
}

/**
 * @brief Converts a move number to board indices.
 * 
 * @param number The move number (1-9).
 * @param row Pointer to the row index.
 * @param col Pointer to the column index.
 */
void getBoardIndices(int number, int *row, int *col) {
    int index = number - 1;  // Convert the 1-based number to a zero-based index (0-8)
    *row = index / SIZE;     // Calculate the row index
    *col = index % SIZE;     // Calculate the column index
}

/**
 * @brief Checks if a player has a winning move.
 * 
 * @param board The Tic-Tac-Toe board.
 * @param player The player's mark ('X' or 'O').
 * @return int Returns 1 if the player has a winning move, 0 otherwise.
 */
int isWinningMove(char board[SIZE][SIZE], char player) {
    for (int i = 0; i < SIZE; i++) {
        if ((board[i][0] == player && board[i][1] == player && board[i][2] == player) ||
            (board[0][i] == player && board[1][i] == player && board[2][i] == player)) {
            return 1;  // Winning row or column
        }
    }

    if ((board[0][0] == player && board[1][1] == player && board[2][2] == player) ||
        (board[0][2] == player && board[1][1] == player && board[2][0] == player)) {
        return 1;  // Winning diagonal
    }

    return 0;  // No winning move found
}

/**
 * @brief Checks if the board is full.
 * 
 * @param board The Tic-Tac-Toe board.
 * @return int Returns 1 if the board is full, 0 otherwise.
 */
int isBoardFull(char board[SIZE][SIZE]) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] == ' ') {
                return 0;  // If any empty space is found, board is not full
            }
        }
    }
    return 1;  // Board is full
}

/**
 * @brief Makes the AI move based on the strategy.
 * 
 * @param board The Tic-Tac-Toe board.
 * @param strategy The strategy string.
 * @param aiMark The AI's mark ('X' or 'O').
 */
void makeAIMove(char board[SIZE][SIZE], const char *strategy, char aiMark) {
    for (int i = 0; i < 9; i++) {
        int row, col;
        getBoardIndices(strategy[i] - '0', &row, &col);  // Get the board indices

        if (board[row][col] == ' ') {
            board[row][col] = aiMark;  // Place the AI's mark on the board
            printf("%d\n", strategy[i] - '0');  // Print the chosen slot number (1-9)
            fflush(stdout);  // Flush after printing the AI move
            return;  // Return after making the move
        }
    }
}

/**
 * @brief Prompts the player to make a move.
 * 
 * @param board The Tic-Tac-Toe board.
 * @param playerMark The player's mark ('X' or 'O').
 */
void makePlayerMove(char board[SIZE][SIZE], char playerMark) {
    int move;
    while (1) {
        printf("Enter your move (1-9): ");
        fflush(stdout);  // Flush after prompting the player
        scanf("%d", &move);

        if (move < 1 || move > 9) {
            printf("Invalid move. Try again.\n");
            fflush(stdout);  // Flush after printing invalid move message
            continue;
        }

        int row, col;
        getBoardIndices(move, &row, &col);  // Convert the move number to row and column indices

        if (board[row][col] == ' ') {
            board[row][col] = playerMark;  // Place the player's mark on the board
            return;  // Exit the function after a valid move is made
        } else {
            printf("That spot is already taken. Try again.\n");
            fflush(stdout);  // Flush after printing spot taken message
        }
    }
}

/**
 * @brief The main function for the Tic-Tac-Toe game.
 * 
 * This function initializes the game, processes command-line arguments, and runs the game loop.
 * The game alternates moves between the AI and the player until there is a winner or a draw.
 * 
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line arguments.
 * @return int Returns 0 on successful execution, 1 on error.
 */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Error1\n");
        fflush(stdout);  // Flush after printing error message
        return 1;
    }

    if (!validateStrategy(argv[1])) {
        printf("Error2\n");
        fflush(stdout);  // Flush after printing error message
        return 1;
    }

    char board[SIZE][SIZE];  // Initialize the Tic-Tac-Toe board
    char strategy[10];
    strcpy(strategy, argv[1]);  // Copy the strategy string to a local variable
    char aiMark = 'X';  // Define the AI's mark
    char playerMark = 'O';  // Define the player's mark

    initializeBoard(board);  // Set up the initial empty board

    while (1) {
        makeAIMove(board, strategy, aiMark);  // Make the AI move based on the strategy
        displayBoard(board);  // Display the current state of the board

        if (isWinningMove(board, aiMark)) {
            printf("AI win\n");
            fflush(stdout);  // Flush after printing AI win message
            break;  // Exit the loop
        }

        if (isBoardFull(board)) {
            printf("DRAW\n");
            fflush(stdout);  // Flush after printing draw message
            break;  // Exit the loop
        }

        makePlayerMove(board, playerMark);  // Prompt the player to make a move
        displayBoard(board);  // Display the current state of the board

        if (isWinningMove(board, playerMark)) {
            printf("AI lost\n");
            fflush(stdout);  // Flush after printing AI lost message
            break;  // Exit the loop
        }

        if (isBoardFull(board)) {
            printf("DRAW\n");
            fflush(stdout);  // Flush after printing draw message
            break;  // Exit the loop
        }
    }

    return 0;  // Return success
}
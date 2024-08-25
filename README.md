
## Part of the Assignment: Tic-Tac-Toe (ttt) Integration

This part of the assignment involves integrating a Tic-Tac-Toe game with a custom network communication tool. The `ttt` program is a simple command-line Tic-Tac-Toe game where two players can make moves by entering a number corresponding to the cell they want to mark on the game board. The game can be executed independently or through a network socket, allowing remote players to compete against each other.

### Key Components:

1. **Tic-Tac-Toe (ttt) Execution**:
    - The `ttt` file is executed with a command-line argument that provides an initial state of the game (e.g., `"123456789"` representing an empty board).
    - The `mync` program is responsible for executing this `ttt` file and managing its input/output through various communication channels.

2. **Communication Channels**:
    - The game can be played over different types of communication channels, such as TCP, UDP, Unix Domain Sockets (both stream and datagram).
    - The `mync` program supports these channels via different command-line options (`-i` for input, `-o` for output).

3. **Interaction with `ttt`**:
    - In the context of this assignment, `ttt` serves as the executable that interacts with the user through the terminal.
    - The `mync` program handles the communication between the user (or remote player) and the `ttt` game by redirecting input and output through the specified network protocol.

### Usage Examples:

- **Stream Connection**: 
  - Server: `./mync -e "./ttt 123456789" -o UDSSS/tmp/mync_stream_server.sock14`
  - Client: `nc -U /tmp/mync_stream_server.sock14`

- **Datagram Connection**:
  - Server: `./mync -e "./ttt 123456789" -i UDSSD/tmp/mync_dgram_server.sock21`
  - Client: `nc -U /tmp/mync_dgram_server.sock21`

In each case, the `ttt` game receives the player's moves and updates the game board, sending back the updated board state or prompts through the communication channel.

---

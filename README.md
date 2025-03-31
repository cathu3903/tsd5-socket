# Gomoku

This is a Gomoku (five-in-a-row) game implemented in **C** using the **SDL2** graphics library, supporting **client-server** network play. Two players can connect over a network to play on the same board, with features like simple stone placement, win detection, and a post-game voting mechanism.

> Built using a client-server structure with sockets, designed for Linux.

## Project Structure

- `client.c`: Client code, built with SDL2 for the graphical interface, responsible for communicating with the server and displaying the board interactively.
- `server.c`: Server code, responsible for accepting two client connections, managing game logic, turns, win detection, and the voting mechanism for restarting the game.
- `commun.h`: Header file containing shared definitions and structures used by both the client and server.
- `Makefile`: Build script to compile the client and server programs.
- `README.md`: Project documentation (this file).

## Features

- 15x15 game board, supporting Black and White players.
- Graphical client interface (using SDL2 + TTF).
- Mouse-click stone placement on the client.
- Win detection (five stones in a row to win).
- Score tracking for winners, with a voting mechanism to decide whether to restart the game.
- Communication via TCP sockets.

## Dependencies

- SDL2
- SDL2_ttf

## Usage Instructions

Game Rules
- Objective: The goal of Gomoku is to be the first player to place five stones in a row, either horizontally, vertically, or diagonally, on a 15x15 grid.
- Players: Two players participate, one playing as Black and the other as White. Black always goes first.

Gameplay:
- Players take turns placing one stone on an empty cell of the board.
- The game ends when one player achieves five stones in a row (winning) or the board is full (draw, though not implemented in this version).
- Scoring: The winner of each game earns 1 point. Scores are displayed on the client interface during the voting phase after each game.

Win Detection:
- If a player places five stones in a row, the game ends, and a message (e.g., "Black wins!") will be displayed in red at the center of the board.

Voting to Restart:
- After a game ends, both players are prompted to vote on whether to play another game ("Play again? (y/n):").
- Enter y to vote for restarting, or n to end the game.
- If both players vote y, the board is reset, scores are updated (winner gets 1 point), and a new game begins.
- If either player votes n, the game ends, and the final scores are displayed.

This version requires libsdl2-dev to be installed.
```
sudo apt install libsdl2-dev
```

and install the sdl2 font libsdl2-ttf-dev
```
sudo apt install libsdl2-ttf-dev
```

Navigate to the exemple directory:
```
cd exemple
```

Compile the project using the provided Makefile:
```
make
```

Run the server in one terminal, specifying the port number:
```
./serverur_TCP # Uses fixed port 12345
```
Run the client in two separate terminals, ensuring the IP address matches the server:
```
./client_TCP localhost #Connects to fixed port 12345
```

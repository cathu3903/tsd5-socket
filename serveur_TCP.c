#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BOARD_SIZE 15   // Board size is 15x15
#define EMPTY 0         // Empty cell
#define BLACK 1         // Black stone
#define WHITE 2         // White stone
#define PORT 12345      // Listening port

// Game state structure, stores board, current turn, two player sockets, scores, etc.
typedef struct {
    int board[BOARD_SIZE][BOARD_SIZE];
    int current_player, socket1, socket2, black_score, white_score;
    int move_state, stone_count[2];
} GameState;

// Initialize board and related state
void init_board(GameState *game) {
    memset(game->board, EMPTY, sizeof(game->board));
    game->current_player = BLACK;
    game->move_state = 0;
    game->stone_count[0] = game->stone_count[1] = 0;
}

// Pack current board state, turn, and score, then send to both players
void send_board(GameState *game) {
    char buffer[1024];
    static int count = 0;
    int len = sprintf(buffer, "BOARD %d %d \n", game->current_player, game->move_state);
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            len += sprintf(buffer + len, "%d ", game->board[i][j]);
    len += sprintf(buffer + len, "%d %d\n", game->black_score, game->white_score);
    write(game->socket1, buffer, len);
    write(game->socket2, buffer, len);
    printf("Sending board to both players... n.%d\n", ++count);
}

// Check if placing a stone at the current position forms five in a row (win condition)
int check_win(GameState *game, int row, int col) {
    int player = game->board[row][col], dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    for (int d = 0; d < 4; d++) {
        int count = 1, dx = dirs[d][0], dy = dirs[d][1];
        // Search in the forward direction
        for (int i = 1; i < 5; i++) {
            int r = row + dx * i, c = col + dy * i;
            if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE || game->board[r][c] != player) break;
            count++;
        }
        // Search in the backward direction
        for (int i = 1; i < 5; i++) {
            int r = row - dx * i, c = col - dy * i;
            if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE || game->board[r][c] != player) break;
            count++;
        }
        if (count >= 5) return 1;
    }
    return 0;
}

// Handle voting mechanism after game ends: return 1 if both players agree to play again
int handle_voting(GameState *game, int winner) {
    char buffer[64], votes[2];
    sprintf(buffer, "VOTE %d %d %d\n", game->black_score, game->white_score, winner);
    write(game->socket1, buffer, strlen(buffer));
    write(game->socket2, buffer, strlen(buffer));
    printf("Sent votes: %s to both players\n", buffer);
    // Read votes from both players
    read(game->socket1, votes, 1);
    read(game->socket2, votes + 1, 1);
    printf("Received votes: %s from both players\n", buffer);
    int result = (votes[0] == 'y' && votes[1] == 'y');
    printf("Voting result: %d\n", result);
    return result;
}

int main() {
    // Create socket, bind, and listen
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr = {AF_INET, htons(PORT), INADDR_ANY};
    bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    listen(sockfd, 2);
    printf("Waiting for players...\n");

    GameState game;
    socklen_t clilen = sizeof(serv_addr);
    // Accept connection from the first player and send identification
    game.socket1 = accept(sockfd, (struct sockaddr *)&serv_addr, &clilen);
    write(game.socket1, "PLAYER 1", 8);
    printf("Player 1 connected\n");
    // Accept connection from the second player and send identification
    game.socket2 = accept(sockfd, (struct sockaddr *)&serv_addr, &clilen);
    write(game.socket2, "PLAYER 2", 8);
    printf("Player 2 connected\n");

    // Initialize board and send initial state
    init_board(&game);
    send_board(&game);

    while (1) {
        char buffer[256];
        // Determine which player's message to read based on current turn
        int sock = game.current_player == BLACK ? game.socket1 : game.socket2;
        int n = read(sock, buffer, sizeof(buffer));
        if (n <= 0) break;
        printf("Received:%s\n", buffer);

        char *command = strtok(buffer, "\n");
        int row, col, fr, fc;
        if (sscanf(command, "MOVE %d %d", &row, &col) == 2 && game.move_state == 0) {
            printf("Handling %s\n", command);
            if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE && game.board[row][col] == EMPTY) {
                game.board[row][col] = game.current_player;
                game.stone_count[game.current_player - 1]++;
                // Check for five in a row
                if (check_win(&game, row, col)) {
                    send_board(&game); 
                    if (handle_voting(&game, game.current_player)) {
                        // If both agree to replay, update score, reset board, continue game
                        game.black_score += game.current_player == BLACK;
                        game.white_score += game.current_player == WHITE;
                        init_board(&game);
                        send_board(&game);
                        continue;
                    }
                    // Otherwise, send END message and exit
                    sprintf(command, "END %d %d", game.black_score, game.white_score);
                    write(game.socket1, command, strlen(command));
                    printf("Sending END to player 1\n");
                    write(game.socket2, command, strlen(command));
                    printf("Sending END to player 2\n");
                    break;
                }
                // Switch turn
                game.current_player = game.current_player == BLACK ? WHITE : BLACK;
                send_board(&game);
            }
        }
    }
    // Close sockets and exit program
    close(game.socket1);
    close(game.socket2);
    close(sockfd);
    return 0;
}

/* Gomoku 服务器 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BOARD_SIZE 15
#define EMPTY 0
#define BLACK 1
#define WHITE 2
#define PORT 12345

typedef struct {
    int board[BOARD_SIZE][BOARD_SIZE];
    int current_player, socket1, socket2, black_score, white_score;
    int move_state, stone_count[2];
} GameState;

void init_board(GameState *game) {
    memset(game->board, EMPTY, sizeof(game->board));
    game->current_player = BLACK;
    game->move_state = 0;
    game->stone_count[0] = game->stone_count[1] = 0;
    game->black_score = game->white_score = 0;
}

void send_board(GameState *game) {
    char buffer[1024];
    int len = sprintf(buffer, "BOARD %d %d ", game->current_player, game->move_state);
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            len += sprintf(buffer + len, "%d ", game->board[i][j]);
    len += sprintf(buffer + len, "%d %d", game->black_score, game->white_score);
    write(game->socket1, buffer, len);
    write(game->socket2, buffer, len);
}

int check_win(GameState *game, int row, int col) {
    int player = game->board[row][col];
    if (player == EMPTY) return 0;
    int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    for (int d = 0; d < 4; d++) {
        int count = 1, dx = dirs[d][0], dy = dirs[d][1];
        for (int i = 1; i < 5; i++) {
            int r = row + dx * i, c = col + dy * i;
            if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE || game->board[r][c] != player) break;
            count++;
        }
        for (int i = 1; i < 5; i++) {
            int r = row - dx * i, c = col - dy * i;
            if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE || game->board[r][c] != player) break;
            count++;
        }
        if (count >= 5) return 1;
    }
    return 0;
}

int handle_voting(GameState *game, int winner) {
    char buffer[64], votes[2];
    sprintf(buffer, "VOTE %d %d %d", game->black_score, game->white_score, winner);
    write(game->socket1, buffer, strlen(buffer));
    write(game->socket2, buffer, strlen(buffer));
    int n1 = read(game->socket1, votes, 1);
    int n2 = read(game->socket2, votes + 1, 1);
    if (n1 <= 0 || n2 <= 0) return 0; // 如果客户端断开，默认不重玩
    return votes[0] == 'y' && votes[1] == 'y';
}

void end_game(GameState *game) {
    char buffer[64];
    sprintf(buffer, "END %d %d", game->black_score, game->white_score);
    write(game->socket1, buffer, strlen(buffer));
    write(game->socket2, buffer, strlen(buffer));
}

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr = {AF_INET, htons(PORT), INADDR_ANY};
    bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    listen(sockfd, 2);
    printf("Waiting for players...\n");

    GameState game;
    socklen_t clilen = sizeof(serv_addr);
    game.socket1 = accept(sockfd, (struct sockaddr *)&serv_addr, &clilen);
    write(game.socket1, "PLAYER 1", 8);
    printf("Player 1 connected\n");
    game.socket2 = accept(sockfd, (struct sockaddr *)&serv_addr, &clilen);
    write(game.socket2, "PLAYER 2", 8);
    printf("Player 2 connected\n");

    init_board(&game);
    send_board(&game);

    char buffer[256];
    while (1) {
        int sock = game.current_player == BLACK ? game.socket1 : game.socket2;
        int n = read(sock, buffer, sizeof(buffer) - 1);
        if (n <= 0) break;
        buffer[n] = '\0'; // 确保字符串以空字符结尾

        int row, col, fr, fc;
        if (sscanf(buffer, "MOVE %d %d", &row, &col) == 2 && game.move_state == 0) {
            if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE && game.board[row][col] == EMPTY) {
                game.board[row][col] = game.current_player;
                game.stone_count[game.current_player - 1]++;
                send_board(&game); // 更新棋盘状态
                if (check_win(&game, row, col)) {
                    game.black_score += (game.current_player == BLACK);
                    game.white_score += (game.current_player == WHITE);
                    send_board(&game); // 显示胜利前的最终状态
                    if (handle_voting(&game, game.current_player)) {
                        init_board(&game);
                        send_board(&game);
                        continue;
                    }
                    end_game(&game);
                    break;
                }
                if (game.stone_count[0] >= 5 && game.stone_count[1] >= 5) game.move_state = 1;
                game.current_player = (game.current_player == BLACK) ? WHITE : BLACK;
                send_board(&game);
            }
        } else if (sscanf(buffer, "MOVE_FROM %d %d MOVE_TO %d %d", &fr, &fc, &row, &col) == 4 && game.move_state == 1) {
            if (fr >= 0 && fr < BOARD_SIZE && fc >= 0 && fc < BOARD_SIZE && row >= 0 && row < BOARD_SIZE &&
                col >= 0 && col < BOARD_SIZE && game.board[fr][fc] == game.current_player && game.board[row][col] == EMPTY) {
                game.board[fr][fc] = EMPTY;
                game.board[row][col] = game.current_player;
                send_board(&game); // 更新棋盘状态
                if (check_win(&game, row, col)) {
                    game.black_score += (game.current_player == BLACK);
                    game.white_score += (game.current_player == WHITE);
                    send_board(&game); // 显示胜利前的最终状态
                    if (handle_voting(&game, game.current_player)) {
                        init_board(&game);
                        send_board(&game);
                        continue;
                    }
                    end_game(&game);
                    break;
                }
                game.current_player = (game.current_player == BLACK) ? WHITE : BLACK;
                send_board(&game);
            }
        }
    }
    close(game.socket1);
    close(game.socket2);
    close(sockfd);
    return 0;
}

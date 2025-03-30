/* Gomoku 服务器 (修改版，只支持下子) */
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
    int current_player;
    int socket1, socket2;
    int black_score, white_score;
} GameState;

void init_board(GameState *game) {
    memset(game->board, EMPTY, sizeof(game->board));
    game->current_player = BLACK;
    game->black_score = game->white_score = 0;
}

void send_board(GameState *game) {
    char buffer[1024];
    // 不再使用 move_state，直接发送当前下子方信息
    int len = sprintf(buffer, "BOARD %d ", game->current_player);
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
    int dirs[4][2] = { {1,0}, {0,1}, {1,1}, {1,-1} };
    for (int d = 0; d < 4; d++) {
        int count = 1, dx = dirs[d][0], dy = dirs[d][1];
        // 正方向
        for (int i = 1; i < 5; i++) {
            int r = row + dx * i, c = col + dy * i;
            if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE || game->board[r][c] != player)
                break;
            count++;
        }
        // 反方向
        for (int i = 1; i < 5; i++) {
            int r = row - dx * i, c = col - dy * i;
            if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE || game->board[r][c] != player)
                break;
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
        // 根据当前下子方选择对应的 socket 读取指令
        int sock = (game.current_player == BLACK) ? game.socket1 : game.socket2;
        int n = read(sock, buffer, sizeof(buffer) - 1);
        if (n <= 0) break;
        buffer[n] = '\0'; // 确保字符串以空字符结尾

        int row, col;
        if (sscanf(buffer, "MOVE %d %d", &row, &col) == 2) {
            if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE && game.board[row][col] == EMPTY) {
                game.board[row][col] = game.current_player;
                if (check_win(&game, row, col)) {
                    // 获胜时更新得分
                    if (game.current_player == BLACK)
                        game.black_score++;
                    else
                        game.white_score++;
                    
                    send_board(&game);  // 更新双方棋盘显示
                    printf("Player %d wins!\n", game.current_player); // 在终端打印胜利结果

                    if (handle_voting(&game, game.current_player)) {
                        init_board(&game);
                        send_board(&game);
                        continue;
                    }
                    end_game(&game);
                    break;
                }
                // 下子后切换玩家
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

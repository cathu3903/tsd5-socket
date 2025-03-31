/* Gomoku 服务器 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BOARD_SIZE 15   // 棋盘大小为15x15
#define EMPTY 0         // 空位置
#define BLACK 1         // 黑子
#define WHITE 2         // 白子
#define PORT 12345      // 监听端口

// 游戏状态结构体，保存棋盘、当前回合、两个玩家的socket、分数等
typedef struct {
    int board[BOARD_SIZE][BOARD_SIZE];
    int current_player, socket1, socket2, black_score, white_score;
    int move_state, stone_count[2];
} GameState;

// 初始化棋盘和相关状态
void init_board(GameState *game) {
    memset(game->board, EMPTY, sizeof(game->board));
    game->current_player = BLACK;
    game->move_state = 0;
    game->stone_count[0] = game->stone_count[1] = 0;
}

// 将当前棋盘状态、回合及分数打包发送给两个玩家
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

// 判断当前位置落子后是否形成五连珠（胜利条件）
int check_win(GameState *game, int row, int col) {
    int player = game->board[row][col], dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    for (int d = 0; d < 4; d++) {
        int count = 1, dx = dirs[d][0], dy = dirs[d][1];
        // 向正方向搜索
        for (int i = 1; i < 5; i++) {
            int r = row + dx * i, c = col + dy * i;
            if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE || game->board[r][c] != player) break;
            count++;
        }
        // 向反方向搜索
        for (int i = 1; i < 5; i++) {
            int r = row - dx * i, c = col - dy * i;
            if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE || game->board[r][c] != player) break;
            count++;
        }
        if (count >= 5) return 1;
    }
    return 0;
}

// 处理游戏结束后的投票机制：返回1表示双方均同意再玩一局
int handle_voting(GameState *game, int winner) {
    char buffer[64], votes[2];
    sprintf(buffer, "VOTE %d %d %d\n", game->black_score, game->white_score, winner);
    write(game->socket1, buffer, strlen(buffer));
    write(game->socket2, buffer, strlen(buffer));
    printf("Sent votes: %s to both players\n", buffer);
    // 分别读取两个玩家的投票
    read(game->socket1, votes, 1);
    read(game->socket2, votes + 1, 1);
    printf("Received votes: %s from both players\n", buffer);
    int result = (votes[0] == 'y' && votes[1] == 'y');
    printf("Voting result: %d\n", result);
    return result;
}

int main() {
    // 创建socket并绑定、监听
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr = {AF_INET, htons(PORT), INADDR_ANY};
    bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    listen(sockfd, 2);
    printf("Waiting for players...\n");

    GameState game;
    socklen_t clilen = sizeof(serv_addr);
    // 接收第一个玩家连接，并发送标识信息
    game.socket1 = accept(sockfd, (struct sockaddr *)&serv_addr, &clilen);
    write(game.socket1, "PLAYER 1", 8);
    printf("Player 1 connected\n");
    // 接收第二个玩家连接，并发送标识信息
    game.socket2 = accept(sockfd, (struct sockaddr *)&serv_addr, &clilen);
    write(game.socket2, "PLAYER 2", 8);
    printf("Player 2 connected\n");

    // 初始化棋盘并发送初始状态
    init_board(&game);
    send_board(&game);

    while (1) {
        char buffer[256];
        // 根据当前回合决定读取哪个玩家的消息
        int sock = game.current_player == BLACK ? game.socket1 : game.socket2;
        int n = read(sock, buffer, sizeof(buffer));
        if (n <= 0) break;
        printf("Received:%s\n", buffer);

        // 分割消息，以换行分隔
        char *command = strtok(buffer, "\n");
        // 如果收到“MOVE”指令且状态允许落子
        int row, col, fr, fc;
        if (sscanf(command, "MOVE %d %d", &row, &col) == 2 && game.move_state == 0) {
            printf("Handling %s\n", command);
            // 检查落子位置合法性
            if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE && game.board[row][col] == EMPTY) {
                game.board[row][col] = game.current_player;
                game.stone_count[game.current_player - 1]++;
                // 检查是否形成五子连珠
                if (check_win(&game, row, col)) {
                    send_board(&game);  // 显示最终落子
                    if (handle_voting(&game, game.current_player)) {
                        // 若双方投票同意，更新分数，重置棋盘，继续游戏
                        game.black_score += game.current_player == BLACK;
                        game.white_score += game.current_player == WHITE;
                        init_board(&game);
                        send_board(&game);
                        continue;
                    }
                    // 否则发送结束消息并退出
                    sprintf(command, "END %d %d", game.black_score, game.white_score);
                    write(game.socket1, command, strlen(command));
                    printf("Sending END to player 1\n");
                    write(game.socket2, command, strlen(command));
                    printf("Sending END to player 2\n");
                    break;
                }
                // 切换回合
                game.current_player = game.current_player == BLACK ? WHITE : BLACK;
                send_board(&game);
            }
        }
    }
    // 关闭socket，结束程序
    close(game.socket1);
    close(game.socket2);
    close(sockfd);
    return 0;
}

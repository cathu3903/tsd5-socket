/*  Application MIROIR  : cote serveur      */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "commun.h"

#define BOARD_SIZE 15
#define EMPTY 0
#define BLACK 1
#define WHITE 2

typedef struct {
    int board[BOARD_SIZE][BOARD_SIZE];
    int current_player;
    int socket1;  // 黑方玩家
    int socket2;  // 白方玩家
} GameState;

void init_board(GameState *game) {
    memset(game->board, EMPTY, sizeof(game->board));
    game->current_player = BLACK;
}

// 发送棋盘状态给两个玩家
void send_board_state(GameState *game) {
    char buffer[1024];
    int len = 0;
    
    // 格式: "BOARD [当前玩家] [棋盘数据]"
    len += sprintf(buffer + len, "BOARD %d ", game->current_player);
    
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            len += sprintf(buffer + len, "%d ", game->board[i][j]);
        }
    }
    
    write(game->socket1, buffer, len);
    write(game->socket2, buffer, len);
}

int check_win(GameState *game, int row, int col) {
    int player = game->board[row][col];
    int directions[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    
    for(int d = 0; d < 4; d++) {
        int count = 1;
        int dx = directions[d][0];
        int dy = directions[d][1];
        
        // 检查正向
        for(int i = 1; i < 5; i++) {
            int new_row = row + dx * i;
            int new_col = col + dy * i;
            if(new_row < 0 || new_row >= BOARD_SIZE || new_col < 0 || new_col >= BOARD_SIZE)
                break;
            if(game->board[new_row][new_col] != player)
                break;
            count++;
        }
        
        // 检查反向
        for(int i = 1; i < 5; i++) {
            int new_row = row - dx * i;
            int new_col = col - dy * i;
            if(new_row < 0 || new_row >= BOARD_SIZE || new_col < 0 || new_col >= BOARD_SIZE)
                break;
            if(game->board[new_row][new_col] != player)
                break;
            count++;
        }
        
        if(count >= 5)
            return 1;
    }
    return 0;
}

void handle_game(GameState *game) {
    char buffer[256];
    int current_socket;
    
    // 发送初始棋盘状态
    send_board_state(game);
    
    while(1) {
        // 确定当前玩家的socket
        current_socket = (game->current_player == BLACK) ? game->socket1 : game->socket2;
        
        // 接收当前玩家的移动
        memset(buffer, 0, sizeof(buffer));
        int n = read(current_socket, buffer, sizeof(buffer));
        
        if (n <= 0) {
            printf("Player disconnected\n");
            break;
        }
        
        int row, col;
        if (sscanf(buffer, "MOVE %d %d", &row, &col) == 2) {
            // 验证移动是否有效
            if(row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE && 
               game->board[row][col] == EMPTY) {
                
                // 更新棋盘
                game->board[row][col] = game->current_player;
                
                // 检查是否获胜
                if(check_win(game, row, col)) {
                    char win_msg[32];
                    sprintf(win_msg, "WIN %d", game->current_player);
                    write(game->socket1, win_msg, strlen(win_msg));
                    write(game->socket2, win_msg, strlen(win_msg));
                    printf("Player %d wins!\n", game->current_player);
                    break;
                }
                
                // 切换玩家
                game->current_player = (game->current_player == BLACK) ? WHITE : BLACK;
                
                // 发送更新后的棋盘状态
                send_board_state(game);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd1, newsockfd2;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    
    // 创建socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(1);
    }
    
    // 设置服务器地址结构
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);
    
    // 绑定socket
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error binding socket");
        exit(1);
    }
    
    // 监听连接
    listen(sockfd, 2);
    printf("Waiting for players...\n");
    
    // 等待两个玩家连接
    newsockfd1 = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    printf("Player 1 (BLACK) connected\n");
    
    // 发送玩家编号
    write(newsockfd1, "PLAYER 1", 8);
    
    newsockfd2 = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    printf("Player 2 (WHITE) connected\n");
    
    // 发送玩家编号
    write(newsockfd2, "PLAYER 2", 8);
    
    // 初始化游戏状态
    GameState game;
    init_board(&game);
    game.socket1 = newsockfd1;
    game.socket2 = newsockfd2;
    
    // 开始游戏
    handle_game(&game);
    
    // 关闭连接
    close(newsockfd1);
    close(newsockfd2);
    close(sockfd);
    
    return 0;
}

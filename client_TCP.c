/* Gomoku 客户端 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define BOARD_SIZE 15                      // 棋盘行数和列数
#define CELL_SIZE 40                       // 每个格子的像素大小
#define WINDOW_SIZE (BOARD_SIZE * CELL_SIZE + 100)  // 窗口总大小
#define EMPTY 0                            // 空格
#define BLACK 1                            // 黑子
#define WHITE 2                            // 白子
#define PORT 12345                         // 服务器端口

// 游戏界面结构体，保存窗口、渲染器、字体、棋盘状态等
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font;
    int board[BOARD_SIZE][BOARD_SIZE];     // 棋盘数据
    int current_player, my_player, black_score, white_score;
    int player_count;                      // 玩家数量（新加入的字段）
    int move_state, from_row, from_col;      // 移动状态及记录起始位置
} GameUI;

// 清理资源函数
void cleanup(GameUI *ui) {
    if (ui->font) TTF_CloseFont(ui->font);
    if (ui->renderer) SDL_DestroyRenderer(ui->renderer);
    if (ui->window) SDL_DestroyWindow(ui->window);
    TTF_Quit();
    SDL_Quit();
}

// 初始化 SDL 窗口、渲染器和字体，设置初始回合为黑子
int init_ui(GameUI *ui) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() < 0) return 0;
    ui->window = SDL_CreateWindow("Gomoku", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  WINDOW_SIZE, WINDOW_SIZE, SDL_WINDOW_SHOWN);
    if (!ui->window) return 0;
    ui->renderer = SDL_CreateRenderer(ui->window, -1, SDL_RENDERER_ACCELERATED);
    if (!ui->renderer) return 0;
    ui->font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 18);
    if (!ui->font) printf("Font not loaded\n");
    ui->move_state = 0;
    // 初始化首个回合为黑子
    ui->current_player = BLACK;
    return 1;
}

// 绘制棋盘、棋子以及状态信息
void draw_board(GameUI *ui) {
    // 绘制棋盘背景
    SDL_SetRenderDrawColor(ui->renderer, 222, 184, 135, 255);
    SDL_RenderClear(ui->renderer);
    // 设置画笔颜色为黑色
    SDL_SetRenderDrawColor(ui->renderer, 0, 0, 0, 255);

    // 绘制横竖网格线
    for (int i = 0; i < BOARD_SIZE; i++) {
        SDL_RenderDrawLine(ui->renderer, 50, 50 + i * CELL_SIZE, 50 + (BOARD_SIZE - 1) * CELL_SIZE, 50 + i * CELL_SIZE);
        SDL_RenderDrawLine(ui->renderer, 50 + i * CELL_SIZE, 50, 50 + i * CELL_SIZE, 50 + (BOARD_SIZE - 1) * CELL_SIZE);
    }
    // 绘制棋子（黑白）
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (ui->board[i][j] != EMPTY) {
                // 根据棋子颜色设置颜色（黑为0，白为255）
                SDL_SetRenderDrawColor(ui->renderer, ui->board[i][j] == BLACK ? 0 : 255,
                                       ui->board[i][j] == BLACK ? 0 : 255,
                                       ui->board[i][j] == BLACK ? 0 : 255, 255);
                int cx = 50 + j * CELL_SIZE, cy = 50 + i * CELL_SIZE, r = CELL_SIZE / 3;
                // 绘制圆形棋子（简单的点阵圆）
                for (int y = -r; y <= r; y++)
                    for (int x = -r; x <= r; x++)
                        if (x * x + y * y <= r * r)
                            SDL_RenderDrawPoint(ui->renderer, cx + x, cy + y);
            }
        }
    }
    // 使用字体渲染并显示文字信息
    if (ui->font) {
        SDL_Color color = {0, 0, 0, 255};
        char text[64];
        // 显示自己身份（黑子或白子）
        sprintf(text, "You are: %s", ui->my_player == BLACK ? "Black" : "White");
        SDL_Surface *surface = TTF_RenderText_Solid(ui->font, text, color);
        if (surface) {
            SDL_Texture *texture = SDL_CreateTextureFromSurface(ui->renderer, surface);
            SDL_Rect rect = {WINDOW_SIZE - 150, 10, surface->w, surface->h};
            SDL_RenderCopy(ui->renderer, texture, NULL, &rect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }
        // 显示当前回合
        sprintf(text, "Turn: %s", ui->current_player == BLACK ? "Black" : "White");
        surface = TTF_RenderText_Solid(ui->font, text, color);
        if (surface) {
            SDL_Texture *texture = SDL_CreateTextureFromSurface(ui->renderer, surface);
            SDL_Rect rect = {10, 10, surface->w, surface->h}; // 放在棋盘顶部
            SDL_RenderCopy(ui->renderer, texture, NULL, &rect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }
        // 显示分数信息
        sprintf(text, "Scores: Black %d, White %d", ui->black_score, ui->white_score);
        surface = TTF_RenderText_Solid(ui->font, text, color);
        if (surface) {
            SDL_Texture *texture = SDL_CreateTextureFromSurface(ui->renderer, surface);
            SDL_Rect rect = {10, WINDOW_SIZE - 30, surface->w, surface->h};
            SDL_RenderCopy(ui->renderer, texture, NULL, &rect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }
    }
    SDL_RenderPresent(ui->renderer);  // 更新渲染内容到窗口
}

// 将像素坐标转换为棋盘的行或列索引
int get_pos(int pixel) { return (pixel - 50 + CELL_SIZE / 2) / CELL_SIZE; }

int main(int argc, char *argv[]) {
    // 参数检查：必须传入服务器主机名
    if (argc < 2) {
        fprintf(stderr, "Usage: %s hostname\n", argv[0]);
        exit(1);
    }
    GameUI ui = {0};
    if (!init_ui(&ui)) { cleanup(&ui); return 1; }

    // 创建 socket 并连接服务器
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct hostent *server = gethostbyname(argv[1]);
    struct sockaddr_in serv_addr = {AF_INET, htons(PORT)};
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connect failed");
        cleanup(&ui);
        exit(1);
    }

    char buffer[1024];
    // 从服务器读取初始信息，判断自己是黑子还是白子
    read(sockfd, buffer, sizeof(buffer));
    ui.my_player = strncmp(buffer, "PLAYER 1", 8) == 0 ? BLACK : WHITE;
    printf("You are %s\n", ui.my_player == BLACK ? "BLACK" : "WHITE");
    memset(ui.board, EMPTY, sizeof(ui.board));
    draw_board(&ui);

    SDL_Event event;
    int running = 1;
    int times = 0;
    // 游戏主循环
    while (running) {
        // 处理 SDL 事件（如退出、鼠标点击）
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = 0;
            // 当鼠标点击且轮到自己操作时
            if (event.type == SDL_MOUSEBUTTONDOWN && ui.current_player == ui.my_player) {
                int row = get_pos(event.button.y), col = get_pos(event.button.x);
                if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE) {
                    if (ui.move_state == 0 && ui.board[row][col] == EMPTY) {
                        // 发出普通落子消息
                        sprintf(buffer, "MOVE %d %d\n", row, col);
                        write(sockfd, buffer, strlen(buffer));
                        printf("Sent MOVE %d %d\n", row, col);
                    } else if (ui.move_state == 1 && ui.board[row][col] == ui.my_player) {
                        // 选择起始点，准备移动棋子
                        ui.from_row = row; ui.from_col = col; ui.move_state = 2;
                        draw_board(&ui);
                    } else if (ui.move_state == 2 && ui.board[row][col] == EMPTY) {
                        // 完成移动棋子的目标位置，并发送消息
                        sprintf(buffer, "MOVE_FROM %d %d MOVE_TO %d %d\n", ui.from_row, ui.from_col, row, col);
                        write(sockfd, buffer, strlen(buffer));
                        printf("Sent MOVE_FROM_TO: %s\n", buffer);
                        ui.move_state = 0;
                    }
                }
            }
        }
        // 设置 socket 读取超时，等待服务器消息
        fd_set fds;
        struct timeval tv = {0, 10000};
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);

        if (select(sockfd + 1, &fds, NULL, NULL, &tv) > 0) {
            memset(buffer, 0, sizeof(buffer));
            // 从服务器读取消息
            int n = read(sockfd, buffer, sizeof(buffer) - 1);
            buffer[n] = '\0';
            printf("Received message: %s\n", buffer);

            // 分解服务器消息，以换行分隔
            char *message = strtok(buffer, "\n");
            while(message != NULL) {
                if (n <= 0) {
                    printf("Server disconnected\n");
                    break;
                }
                printf("Handling message: %s\n", message);
                if (strncmp(message, "BOARD", 5) == 0) {
                    // 处理 BOARD 消息，更新棋盘、当前回合、状态和分数
                    printf("Received board %d times;\n", ++times);
                    int pos = 6;
                    sscanf(message + pos, "%d %d", &ui.current_player, &ui.move_state);
                    while (message[pos] != ' ' && pos < n) pos++; pos++;
                    while (message[pos] != ' ' && pos < n) pos++; pos++;
                    for (int i = 0; i < BOARD_SIZE; i++)
                        for (int j = 0; j < BOARD_SIZE; j++) {
                            sscanf(message + pos, "%d", &ui.board[i][j]);
                            while (message[pos] != ' ' && pos < n) pos++; pos++;
                        }
                    sscanf(message + pos, "%d %d", &ui.black_score, &ui.white_score);
                    draw_board(&ui);
                } else if (strncmp(message, "VOTE", 4) == 0) {
                    // 处理 VOTE 消息，显示分数及胜者，等待用户输入是否再来一局
                    printf("VOTE\n");
                    int winner;
                    sscanf(message + 5, "%d %d %d", &ui.black_score, &ui.white_score, &winner);
                    draw_board(&ui);
                    if (winner)
                        printf("%s wins!\n", winner == BLACK ? "BLACK" : "WHITE");
                    printf("Play again? (y/n): ");
                    char vote;
                    scanf(" %c", &vote);
                    write(sockfd, &vote, 1);
                    printf("Sending vote...\n");
                } else if (strncmp(message, "END", 3) == 0) {
                    // 处理 END 消息，游戏结束，显示最后比分后退出
                    printf("Game ended\n");
                    sscanf(message + 4, "%d %d", &ui.black_score, &ui.white_score);
                    draw_board(&ui);
                    SDL_Delay(2000);
                    running = 0;
                }
                // 处理下一个消息
                message = strtok(NULL, "\n");
            }
        }
        SDL_Delay(10);  // 延时，降低 CPU 占用
    }
    close(sockfd);
    cleanup(&ui);
    return 0;
}

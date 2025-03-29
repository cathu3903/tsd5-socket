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

#define BOARD_SIZE 15
#define CELL_SIZE 40
#define WINDOW_SIZE (BOARD_SIZE * CELL_SIZE + 100)
#define EMPTY 0
#define BLACK 1
#define WHITE 2
#define PORT 12345

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font;
    int board[BOARD_SIZE][BOARD_SIZE];
    int current_player, my_player, black_score, white_score;
    int move_state, from_row, from_col;
} GameUI;

void cleanup(GameUI *ui) {
    if (ui->font) TTF_CloseFont(ui->font);
    if (ui->renderer) SDL_DestroyRenderer(ui->renderer);
    if (ui->window) SDL_DestroyWindow(ui->window);
    TTF_Quit();
    SDL_Quit();
}

int init_ui(GameUI *ui) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() < 0) return 0;
    ui->window = SDL_CreateWindow("Gomoku", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  WINDOW_SIZE, WINDOW_SIZE, SDL_WINDOW_SHOWN);
    if (!ui->window) return 0;
    ui->renderer = SDL_CreateRenderer(ui->window, -1, SDL_RENDERER_ACCELERATED);
    if (!ui->renderer) return 0;
    ui->font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 18); // Linux 常用字体
    if (!ui->font) printf("Font not loaded\n");
    ui->move_state = 0;
    return 1;
}

void draw_board(GameUI *ui) {
    SDL_SetRenderDrawColor(ui->renderer, 222, 184, 135, 255);
    SDL_RenderClear(ui->renderer);
    SDL_SetRenderDrawColor(ui->renderer, 0, 0, 0, 255);
    for (int i = 0; i < BOARD_SIZE; i++) {
        SDL_RenderDrawLine(ui->renderer, 50, 50 + i * CELL_SIZE, 50 + (BOARD_SIZE - 1) * CELL_SIZE, 50 + i * CELL_SIZE);
        SDL_RenderDrawLine(ui->renderer, 50 + i * CELL_SIZE, 50, 50 + i * CELL_SIZE, 50 + (BOARD_SIZE - 1) * CELL_SIZE);
    }
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (ui->board[i][j] != EMPTY) {
                SDL_SetRenderDrawColor(ui->renderer, ui->board[i][j] == BLACK ? 0 : 255, ui->board[i][j] == BLACK ? 0 : 255, ui->board[i][j] == BLACK ? 0 : 255, 255);
                int cx = 50 + j * CELL_SIZE, cy = 50 + i * CELL_SIZE, r = CELL_SIZE / 3;
                for (int y = -r; y <= r; y++)
                    for (int x = -r; x <= r; x++)
                        if (x * x + y * y <= r * r) SDL_RenderDrawPoint(ui->renderer, cx + x, cy + y);
            }
        }
    }
    if (ui->font) {
        SDL_Color color = {0, 0, 0, 255};
        char text[64];
        sprintf(text, "Player: %s | Scores: B%d W%d", ui->current_player == BLACK ? "Black" : "White", ui->black_score, ui->white_score);
        SDL_Surface *surface = TTF_RenderText_Solid(ui->font, text, color);
        if (surface) {
            SDL_Texture *texture = SDL_CreateTextureFromSurface(ui->renderer, surface);
            SDL_Rect rect = {10, WINDOW_SIZE - 30, surface->w, surface->h};
            SDL_RenderCopy(ui->renderer, texture, NULL, &rect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }
    }
    SDL_RenderPresent(ui->renderer);
}

int get_pos(int pixel) { return (pixel - 50 + CELL_SIZE / 2) / CELL_SIZE; }

int main(int argc, char *argv[]) {
    if (argc < 2) { fprintf(stderr, "Usage: %s hostname\n", argv[0]); exit(1); }
    GameUI ui = {0};
    if (!init_ui(&ui)) { cleanup(&ui); return 1; }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct hostent *server = gethostbyname(argv[1]);
    struct sockaddr_in serv_addr = {AF_INET, htons(PORT)};
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { perror("Connect failed"); cleanup(&ui); exit(1); }

    char buffer[1024];
    read(sockfd, buffer, sizeof(buffer));
    ui.my_player = strncmp(buffer, "PLAYER 1", 8) == 0 ? BLACK : WHITE;
    printf("You are %s\n", ui.my_player == BLACK ? "BLACK" : "WHITE");
    memset(ui.board, EMPTY, sizeof(ui.board));
    draw_board(&ui);

    SDL_Event event;
    int running = 1;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
            if (event.type == SDL_MOUSEBUTTONDOWN && ui.current_player == ui.my_player) {
                int row = get_pos(event.button.y), col = get_pos(event.button.x);
                if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE) {
                    if (ui.move_state == 0 && ui.board[row][col] == EMPTY) {
                        sprintf(buffer, "MOVE %d %d", row, col);
                        write(sockfd, buffer, strlen(buffer));
                    } else if (ui.move_state == 1 && ui.board[row][col] == ui.my_player) {
                        ui.from_row = row; ui.from_col = col; ui.move_state = 2;
                        draw_board(&ui);
                    } else if (ui.move_state == 2 && ui.board[row][col] == EMPTY) {
                        sprintf(buffer, "MOVE_FROM %d %d MOVE_TO %d %d", ui.from_row, ui.from_col, row, col);
                        write(sockfd, buffer, strlen(buffer));
                        ui.move_state = 0;
                    }
                }
            }
        }
        fd_set fds;
        struct timeval tv = {0, 10000};
        FD_ZERO(&fds); FD_SET(sockfd, &fds);
        if (select(sockfd + 1, &fds, NULL, NULL, &tv) > 0) {
            memset(buffer, 0, sizeof(buffer));
            int n = read(sockfd, buffer, sizeof(buffer));
            if (n <= 0) { printf("Server disconnected\n"); break; }
            if (strncmp(buffer, "BOARD", 5) == 0) {
                int pos = 6;
                sscanf(buffer + pos, "%d %d", &ui.current_player, &ui.move_state);
                while (buffer[pos] != ' ' && pos < n) pos++; pos++;
                while (buffer[pos] != ' ' && pos < n) pos++; pos++;
                for (int i = 0; i < BOARD_SIZE; i++)
                    for (int j = 0; j < BOARD_SIZE; j++) {
                        sscanf(buffer + pos, "%d", &ui.board[i][j]);
                        while (buffer[pos] != ' ' && pos < n) pos++; pos++;
                    }
                sscanf(buffer + pos, "%d %d", &ui.black_score, &ui.white_score);
                draw_board(&ui);
            } else if (strncmp(buffer, "WIN", 3) == 0) {
                int winner;
                sscanf(buffer + 4, "%d", &winner);
                draw_board(&ui); // 显示最后一个落子
                printf("%s wins!\n", winner == BLACK ? "BLACK" : "WHITE");
                SDL_Delay(2000);
            } else if (strncmp(buffer, "VOTE", 4) == 0) {
                sscanf(buffer + 5, "%d %d", &ui.black_score, &ui.white_score);
                draw_board(&ui);
                printf("Play again? (y/n): ");
                char vote;
                scanf(" %c", &vote);
                write(sockfd, &vote, 1);
            } else if (strncmp(buffer, "END", 3) == 0) {
                sscanf(buffer + 4, "%d %d", &ui.black_score, &ui.white_score);
                draw_board(&ui);
                printf("Game ended. Scores: B%d W%d\n", ui.black_score, ui.white_score);
                SDL_Delay(2000);
                running = 0;
            }
        }
        SDL_Delay(10);
    }
    close(sockfd);
    cleanup(&ui);
    return 0;
}
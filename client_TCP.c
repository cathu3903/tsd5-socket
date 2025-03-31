#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define BOARD_SIZE 15                      // Number of rows and columns on the board
#define CELL_SIZE 40                       // Pixel size for each cell
#define WINDOW_SIZE (BOARD_SIZE * CELL_SIZE + 100)  // Total window size
#define EMPTY 0                            // Empty cell
#define BLACK 1                            // Black stone
#define WHITE 2                            // White stone
#define PORT 12345                         // server port

// Game UI structure, stores window, renderer, font, board state, etc.
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font;
    int board[BOARD_SIZE][BOARD_SIZE];     // Board data
    int current_player, my_player, black_score, white_score;
    int player_count;                      // Number of players
    int move_state, from_row, from_col;      // Move state and starting position
} GameUI;

// Resource cleanup function
void cleanup(GameUI *ui) {
    if (ui->font) TTF_CloseFont(ui->font);
    if (ui->renderer) SDL_DestroyRenderer(ui->renderer);
    if (ui->window) SDL_DestroyWindow(ui->window);
    TTF_Quit();
    SDL_Quit();
}

// Initialize SDL window, renderer, and font, set initial turn as black
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
    ui->current_player = BLACK;
    return 1;
}

// Draw the board, pieces, and status information
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
                SDL_SetRenderDrawColor(ui->renderer, ui->board[i][j] == BLACK ? 0 : 255,
                                       ui->board[i][j] == BLACK ? 0 : 255,
                                       ui->board[i][j] == BLACK ? 0 : 255, 255);
                int cx = 50 + j * CELL_SIZE, cy = 50 + i * CELL_SIZE, r = CELL_SIZE / 3;
                for (int y = -r; y <= r; y++)
                    for (int x = -r; x <= r; x++)
                        if (x * x + y * y <= r * r)
                            SDL_RenderDrawPoint(ui->renderer, cx + x, cy + y);
            }
        }
    }
    if (ui->font) {
        SDL_Color color = {0, 0, 0, 255};
        char text[64];
        // Display player identity
        sprintf(text, "You are: %s", ui->my_player == BLACK ? "Black" : "White");
        SDL_Surface *surface = TTF_RenderText_Solid(ui->font, text, color);
        if (surface) {
            SDL_Texture *texture = SDL_CreateTextureFromSurface(ui->renderer, surface);
            SDL_Rect rect = {WINDOW_SIZE - 150, 10, surface->w, surface->h};
            SDL_RenderCopy(ui->renderer, texture, NULL, &rect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }
        // Display current turn
        sprintf(text, "Turn: %s", ui->current_player == BLACK ? "Black" : "White");
        surface = TTF_RenderText_Solid(ui->font, text, color);
        if (surface) {
            SDL_Texture *texture = SDL_CreateTextureFromSurface(ui->renderer, surface);
            SDL_Rect rect = {10, 10, surface->w, surface->h}; // 放在棋盘顶部
            SDL_RenderCopy(ui->renderer, texture, NULL, &rect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }
        // Display score information
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
    SDL_RenderPresent(ui->renderer);  // Update render content to the window
}

// Convert pixel coordinate to board row or column index
int get_pos(int pixel) { return (pixel - 50 + CELL_SIZE / 2) / CELL_SIZE; }

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s hostname\n", argv[0]);
        exit(1);
    }
    GameUI ui = {0};
    if (!init_ui(&ui)) { cleanup(&ui); return 1; }

    // Create socket and connect to server
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
    // Read initial message from server, determine whether self is black or white
    read(sockfd, buffer, sizeof(buffer));
    ui.my_player = strncmp(buffer, "PLAYER 1", 8) == 0 ? BLACK : WHITE;
    printf("You are %s\n", ui.my_player == BLACK ? "BLACK" : "WHITE");
    memset(ui.board, EMPTY, sizeof(ui.board));
    draw_board(&ui);

    SDL_Event event;
    int running = 1;
    int times = 0;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = 0;
            // On mouse click and it's your turn
            if (event.type == SDL_MOUSEBUTTONDOWN && ui.current_player == ui.my_player) {
                int row = get_pos(event.button.y), col = get_pos(event.button.x);
                if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE) {
                    if (ui.move_state == 0 && ui.board[row][col] == EMPTY) {
                        // Send normal move message
                        sprintf(buffer, "MOVE %d %d\n", row, col);
                        write(sockfd, buffer, strlen(buffer));
                        printf("Sent MOVE %d %d\n", row, col);
                    } else if (ui.move_state == 1 && ui.board[row][col] == ui.my_player) {
                        // Choose starting point, prepare to move piece
                        ui.from_row = row; ui.from_col = col; ui.move_state = 2;
                        draw_board(&ui);
                    } else if (ui.move_state == 2 && ui.board[row][col] == EMPTY) {
                        // Complete target move and send message
                        sprintf(buffer, "MOVE_FROM %d %d MOVE_TO %d %d\n", ui.from_row, ui.from_col, row, col);
                        write(sockfd, buffer, strlen(buffer));
                        printf("Sent MOVE_FROM_TO: %s\n", buffer);
                        ui.move_state = 0;
                    }
                }
            }
        }
        // Set socket read timeout and wait for server message
        fd_set fds;
        struct timeval tv = {0, 10000};
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);

        if (select(sockfd + 1, &fds, NULL, NULL, &tv) > 0) {
            memset(buffer, 0, sizeof(buffer));
            // Read message from server
            int n = read(sockfd, buffer, sizeof(buffer) - 1);
            buffer[n] = '\0';
            printf("Received message: %s\n", buffer);

            // Split server message by newline
            char *message = strtok(buffer, "\n");
            while(message != NULL) {
                if (n <= 0) {
                    printf("Server disconnected\n");
                    break;
                }
                printf("Handling message: %s\n", message);
                if (strncmp(message, "BOARD", 5) == 0) {
                    // Handle BOARD message, update board, current turn, state and scores
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
                    // Handle VOTE message, show scores and winner, prompt user to play again
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
                    //  Handle END message, game ends, show final scores and exit
                    printf("Game ended\n");
                    sscanf(message + 4, "%d %d", &ui.black_score, &ui.white_score);
                    draw_board(&ui);
                    SDL_Delay(2000);
                    running = 0;
                }
                // Handle next message
                message = strtok(NULL, "\n");
            }
        }
        SDL_Delay(10); 
    }
    close(sockfd);
    cleanup(&ui);
    return 0;
}

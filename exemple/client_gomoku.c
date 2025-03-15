/*	Application MIROIR  : cote client		*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "commun.h"

#define BOARD_SIZE 15
#define EMPTY 0
#define BLACK 1
#define WHITE 2
#define CELL_SIZE 40
#define WINDOW_SIZE (BOARD_SIZE * CELL_SIZE + 100)

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font;
    int board[BOARD_SIZE][BOARD_SIZE];
    int current_player;
    int my_player;  // 1 = Black, 2 = White
} GameUI;

void cleanup(GameUI *ui) {
    if (ui->font) TTF_CloseFont(ui->font);
    if (ui->renderer) SDL_DestroyRenderer(ui->renderer);
    if (ui->window) SDL_DestroyWindow(ui->window);
    TTF_Quit();
    SDL_Quit();
}

int init_ui(GameUI *ui) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 0;
    }

    if (TTF_Init() < 0) {
        printf("TTF initialization failed: %s\n", TTF_GetError());
        return 0;
    }

    ui->window = SDL_CreateWindow("Gomoku",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_SIZE, WINDOW_SIZE,
        SDL_WINDOW_SHOWN);
    if (!ui->window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        return 0;
    }

    ui->renderer = SDL_CreateRenderer(ui->window, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ui->renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        return 0;
    }

    // Common font paths for macOS (English fonts)
    const char* font_paths[] = {
        "/System/Library/Fonts/Helvetica.ttc",
        "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Courier New.ttf",
        "/System/Library/Fonts/Times.ttc"
    };
    
    ui->font = NULL;
    for (size_t i = 0; i < sizeof(font_paths)/sizeof(font_paths[0]); i++) {
        ui->font = TTF_OpenFont(font_paths[i], 18);
        if (ui->font) break;
    }
    
    if (!ui->font) {
        printf("Warning: Could not load any font, text will not be displayed\n");
        // Continue running even without a font
    }

    return 1;
}

void draw_board(GameUI *ui) {
    // Set background color
    SDL_SetRenderDrawColor(ui->renderer, 222, 184, 135, 255);
    SDL_RenderClear(ui->renderer);

    // Draw board lines
    SDL_SetRenderDrawColor(ui->renderer, 0, 0, 0, 255);
    for (int i = 0; i < BOARD_SIZE; i++) {
        // Horizontal lines
        SDL_RenderDrawLine(ui->renderer,
            50, 50 + i * CELL_SIZE,
            50 + (BOARD_SIZE - 1) * CELL_SIZE, 50 + i * CELL_SIZE);
        // Vertical lines
        SDL_RenderDrawLine(ui->renderer,
            50 + i * CELL_SIZE, 50,
            50 + i * CELL_SIZE, 50 + (BOARD_SIZE - 1) * CELL_SIZE);
    }

    // Draw stones
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (ui->board[i][j] != EMPTY) {
                if (ui->board[i][j] == BLACK) {
                    SDL_SetRenderDrawColor(ui->renderer, 0, 0, 0, 255);
                } else {
                    SDL_SetRenderDrawColor(ui->renderer, 255, 255, 255, 255);
                }
                
                // Draw circular stone
                int centerX = 50 + j * CELL_SIZE;
                int centerY = 50 + i * CELL_SIZE;
                int radius = CELL_SIZE/3;
                
                for (int y = -radius; y <= radius; y++) {
                    for (int x = -radius; x <= radius; x++) {
                        if (x*x + y*y <= radius*radius) {
                            SDL_RenderDrawPoint(ui->renderer, centerX + x, centerY + y);
                        }
                    }
                }
            }
        }
    }

    // Display current player and my role
    if (ui->font) {
        SDL_Color textColor = {0, 0, 0, 255};
        char text[64];
        
        // Display current player
        sprintf(text, "Current Player: %s", ui->current_player == BLACK ? "Black" : "White");
        SDL_Surface *surface = TTF_RenderText_Solid(ui->font, text, textColor);
        if (surface) {
            SDL_Texture *texture = SDL_CreateTextureFromSurface(ui->renderer, surface);
            SDL_Rect textRect = {10, WINDOW_SIZE - 60, surface->w, surface->h};
            SDL_RenderCopy(ui->renderer, texture, NULL, &textRect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }
        
        // Display my role
        sprintf(text, "I am: %s", ui->my_player == BLACK ? "Black" : "White");
        surface = TTF_RenderText_Solid(ui->font, text, textColor);
        if (surface) {
            SDL_Texture *texture = SDL_CreateTextureFromSurface(ui->renderer, surface);
            SDL_Rect textRect = {10, WINDOW_SIZE - 30, surface->w, surface->h};
            SDL_RenderCopy(ui->renderer, texture, NULL, &textRect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }
    }

    SDL_RenderPresent(ui->renderer);
}

int get_board_position(int pixel_pos) {
    return (pixel_pos - 50 + CELL_SIZE/2) / CELL_SIZE;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s hostname\n", argv[0]);
        exit(1);
    }

    GameUI ui = {0};
    if (!init_ui(&ui)) {
        cleanup(&ui);
        return 1;
    }

    // Network connection
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[1024];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        cleanup(&ui);
        exit(1);
    }

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "Error, no such host\n");
        cleanup(&ui);
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(PORT);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error connecting to server");
        cleanup(&ui);
        exit(1);
    }

    printf("Connected to server. Waiting for other player...\n");
    
    // Receive player number
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_read = read(sockfd, buffer, sizeof(buffer));
    if (bytes_read <= 0) {
        perror("Error reading from server");
        cleanup(&ui);
        close(sockfd);
        exit(1);
    }
    
    if (strncmp(buffer, "PLAYER 1", 8) == 0) {
        ui.my_player = BLACK;
        printf("You are Player 1 (BLACK)\n");
    } else {
        ui.my_player = WHITE;
        printf("You are Player 2 (WHITE)\n");
    }

    SDL_Event event;
    int running = 1;

    while (running) {
        // Handle SDL events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
                break;
            }
            
            // If it's my turn, handle mouse clicks
            if (event.type == SDL_MOUSEBUTTONDOWN && ui.current_player == ui.my_player) {
                int row = get_board_position(event.button.y);
                int col = get_board_position(event.button.x);
                
                if (row >= 0 && row < BOARD_SIZE && 
                    col >= 0 && col < BOARD_SIZE && 
                    ui.board[row][col] == EMPTY) {
                    
                    // Send move to server
                    memset(buffer, 0, sizeof(buffer));
                    int len = snprintf(buffer, sizeof(buffer), "MOVE %d %d", row, col);
                    if (write(sockfd, buffer, len) < 0) {
                        perror("Error writing to server");
                        running = 0;
                        break;
                    }
                }
            }
        }
        
        // Receive game state
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 10000; // 10ms timeout
        
        if (select(sockfd + 1, &readfds, NULL, NULL, &tv) > 0) {
            memset(buffer, 0, sizeof(buffer));
            ssize_t n = read(sockfd, buffer, sizeof(buffer));
            
            if (n <= 0) {
                printf("Server disconnected\n");
                break;
            }
            
            if (strncmp(buffer, "BOARD", 5) == 0) {
                // Parse board state
                int pos = 6; // Skip "BOARD "
                sscanf(buffer + pos, "%d", &ui.current_player);
                
                // Skip current player number
                while (buffer[pos] != ' ' && pos < (int)n) pos++;
                pos++; // Skip space
                
                // Parse board
                for (int i = 0; i < BOARD_SIZE; i++) {
                    for (int j = 0; j < BOARD_SIZE; j++) {
                        sscanf(buffer + pos, "%d", &ui.board[i][j]);
                        
                        // Move to next number
                        while (buffer[pos] != ' ' && pos < (int)n) pos++;
                        pos++; // Skip space
                    }
                }
                
                // Update display
                draw_board(&ui);
            }
            else if (strncmp(buffer, "WIN", 3) == 0) {
                int winner;
                sscanf(buffer + 4, "%d", &winner);
                
                // Update display
                draw_board(&ui);
                
                // Display win message
                char win_text[64];
                sprintf(win_text, "%s wins!", winner == BLACK ? "Black" : "White");
                
                if (ui.font) {
                    SDL_Color textColor = {255, 0, 0, 255};
                    SDL_Surface *surface = TTF_RenderText_Solid(ui.font, win_text, textColor);
                    if (surface) {
                        SDL_Texture *texture = SDL_CreateTextureFromSurface(ui.renderer, surface);
                        
                        int textW = surface->w;
                        int textH = surface->h;
                        SDL_Rect textRect = {
                            (WINDOW_SIZE - textW) / 2,
                            (WINDOW_SIZE - textH) / 2,
                            textW, textH
                        };
                        
                        SDL_RenderCopy(ui.renderer, texture, NULL, &textRect);
                        SDL_RenderPresent(ui.renderer);
                        
                        SDL_FreeSurface(surface);
                        SDL_DestroyTexture(texture);
                    }
                }
                
                printf("%s wins!\n", winner == BLACK ? "BLACK" : "WHITE");
                SDL_Delay(3000);  // Display for 3 seconds before exiting
                running = 0;
            }
        }
        
        SDL_Delay(10); // Reduce CPU usage
    }

    close(sockfd);
    cleanup(&ui);
    return 0;
}

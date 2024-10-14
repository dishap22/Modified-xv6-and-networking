#include "header.h"

char board[BOARD_SIZE][BOARD_SIZE];
Player players[2];
int current_player = 0;
int player_count = 0;
int game_over = 0;
pthread_mutex_t lock;
pthread_cond_t cond;

void initialize_board() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = EMPTY;
        }
    }
}

void print_board() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            printf("%c ", board[i][j]);
        }
        printf("\n");
    }
}

int check_winner() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != EMPTY)
            return 1;
        if (board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[0][i] != EMPTY)
            return 1;
    }
    if (board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != EMPTY)
        return 1;
    if (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != EMPTY)
        return 1;
    return 0;
}

int is_draw() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] == EMPTY)
                return 0;
        }
    }
    return 1;
}

void send_board(Player *player) {
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "Current board:\n");
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            snprintf(message + strlen(message), sizeof(message) - strlen(message), "%c ", board[i][j]);
        }
        strcat(message, "\n");
    }
    send(player->socket, message, strlen(message), 0);
}

void *handle_client(void *arg) {
    Player *player = (Player *)arg;
    char buffer[BUFFER_SIZE];
    int row, col;

    while (1) {
        // Game loop
        game_over = 0;
        while (!game_over) {
            pthread_mutex_lock(&lock);
            while (current_player != (player->symbol == PLAYER1 ? 0 : 1)) {
                pthread_cond_wait(&cond, &lock);
            }

            // Inform the player it's their turn
            snprintf(buffer, sizeof(buffer), "Your turn, Player %c. Enter your move (row col):\n", player->symbol);
            send(player->socket, buffer, strlen(buffer), 0);

            // Receive move
            int received = recv(player->socket, buffer, sizeof(buffer), 0);
            if (received <= 0) {
                printf("Player disconnected.\n");
                game_over = 1;
                break;
            }
            buffer[received] = '\0';  // Ensure null-termination
            sscanf(buffer, "%d %d", &row, &col);

            if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE && board[row][col] == EMPTY) {
                board[row][col] = player->symbol;

                if (check_winner()) {
                    game_over = 1;
                    send_board(&players[0]);
                    send_board(&players[1]);
                    snprintf(buffer, BUFFER_SIZE, "Player %c Wins!\n", player->symbol);
                    send(players[0].socket, buffer, strlen(buffer), 0);
                    send(players[1].socket, buffer, strlen(buffer), 0);
                } else if (is_draw()) {
                    game_over = 1;
                    send_board(&players[0]);
                    send_board(&players[1]);
                    snprintf(buffer, BUFFER_SIZE, "It's a Draw!\n");
                    send(players[0].socket, buffer, strlen(buffer), 0);
                    send(players[1].socket, buffer, strlen(buffer), 0);
                } else {
                    current_player = (current_player + 1) % 2;
                    send_board(&players[0]);
                    send_board(&players[1]);
                    snprintf(buffer, BUFFER_SIZE, "Player %c's turn\n", players[current_player].symbol);
                    send(players[current_player].socket, buffer, strlen(buffer), 0);
                }
            } else {
                snprintf(buffer, BUFFER_SIZE, "Invalid move, try again.\n");
                send(player->socket, buffer, strlen(buffer), 0);
            }
            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&lock);
        }

        // After game ends, ask Player 1 if they want to continue
        if (player->symbol == PLAYER1) {
            snprintf(buffer, BUFFER_SIZE, "Do you want to play again? (yes/no)\n");
            send(player->socket, buffer, strlen(buffer), 0);
            int received = recv(player->socket, buffer, sizeof(buffer), 0);
            buffer[received] = '\0';  // Ensure null-termination
            if (strncmp(buffer, "yes", 3) != 0) {
                printf("Player 1 has chosen to exit.\n");
                break;
            }

            // Ask Player 2
            snprintf(buffer, BUFFER_SIZE, "Player 1 wants to play again. Do you want to play again? (yes/no)\n");
            send(players[1].socket, buffer, strlen(buffer), 0);
            received = recv(players[1].socket, buffer, sizeof(buffer), 0);
            buffer[received] = '\0';  // Ensure null-termination
            if (strncmp(buffer, "yes", 3) != 0) {
                printf("Player 2 has chosen to exit.\n");
                break;
            }
        }

        // Reset the game state for the next game
        initialize_board();
        current_player = 0;  // Reset to Player 1
    }

    // Close the player socket
    close(player->socket);
    close(players[!player_count].socket);
    return NULL;
}

int main() {
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t tid[2];

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);
    initialize_board();

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 2) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server started, waiting for players to connect...\n");

    for (int i = 0; i < 2; i++) {
        players[i].socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (players[i].socket < 0) {
            perror("Accept failed");
            close(server_socket);
            exit(EXIT_FAILURE);
        }
        players[i].symbol = (i == 0) ? PLAYER1 : PLAYER2;
        printf("Player %d connected\n", i + 1);
        pthread_create(&tid[i], NULL, handle_client, &players[i]);
    }

    for (int i = 0; i < 2; i++) {
        pthread_join(tid[i], NULL);
    }

    close(server_socket);
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);
    return 0;
}

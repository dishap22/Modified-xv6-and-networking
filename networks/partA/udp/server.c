#include "header.h"

char board[BOARD_SIZE][BOARD_SIZE];
Player players[2];
int current_player = 0;
int player_count = 0;
int game_over = 0;
pthread_mutex_t lock;

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

void send_board(Player *player, int socket) {
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "Current board:\n");
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            snprintf(message + strlen(message), sizeof(message) - strlen(message), "%c ", board[i][j]);
        }
        strcat(message, "\n");
    }
    sendto(socket, message, strlen(message), 0, (struct sockaddr*)&player->addr, sizeof(player->addr));
}

void *handle_client(void *arg) {
    int socket = *((int *)arg);
    char buffer[BUFFER_SIZE];
    int row, col;
    socklen_t addr_len = sizeof(players[0].addr);
    Player *current_player_ptr;

    while (player_count < 2) {
        recvfrom(socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&players[player_count].addr, &addr_len);
        players[player_count].symbol = (player_count == 0) ? PLAYER1 : PLAYER2;
        printf("Player %d connected\n", player_count + 1);
        player_count++;
    }

    while (1) {
        game_over = 0;
        current_player = 0;
        initialize_board();
        int received;
        
        while (!game_over) {
            pthread_mutex_lock(&lock);
            current_player_ptr = &players[current_player];
            snprintf(buffer, BUFFER_SIZE, "Your turn, Player %c. Enter your move (row col):\n", current_player_ptr->symbol);
            sendto(socket, buffer, strlen(buffer), 0, (struct sockaddr*)&current_player_ptr->addr, sizeof(current_player_ptr->addr));

            received = recvfrom(socket, buffer, sizeof(buffer), 0, NULL, NULL);
            buffer[received] = '\0'; 
            sscanf(buffer, "%d %d", &row, &col);

            if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE && board[row][col] == EMPTY) {
                board[row][col] = current_player_ptr->symbol;

                if (check_winner()) {
                    game_over = 1;
                    snprintf(buffer, BUFFER_SIZE, "Player %c Wins!\n", current_player_ptr->symbol);
                } else if (is_draw()) {
                    game_over = 1;
                    snprintf(buffer, BUFFER_SIZE, "It's a Draw!\n");
                } else {
                    current_player = (current_player + 1) % 2;
                    snprintf(buffer, BUFFER_SIZE, "Player %c's turn\n", players[current_player].symbol);
                }
            } else {
                snprintf(buffer, BUFFER_SIZE, "Invalid move, try again.\n");
            }

            send_board(&players[0], socket);
            send_board(&players[1], socket);
            sendto(socket, buffer, strlen(buffer), 0, (struct sockaddr*)&current_player_ptr->addr, sizeof(current_player_ptr->addr));
            pthread_mutex_unlock(&lock);
        }

        for (int i = 0; i < 2; i++) {
            snprintf(buffer, BUFFER_SIZE, "Do you want to play again? (yes/no)\n");
            sendto(socket, buffer, strlen(buffer), 0, (struct sockaddr*)&players[i].addr, sizeof(players[i].addr));
            recvfrom(socket, buffer, sizeof(buffer), 0, NULL, NULL);
            buffer[received] = '\0'; 

            if (strncmp(buffer, "no", 2) == 0) {
                printf("Player %d has chosen to exit.\n", i + 1);
                return NULL;
            }
        }
    }
    return NULL;
}

int main() {
    int server_socket;
    struct sockaddr_in server_addr;
    pthread_t tid;

    pthread_mutex_init(&lock, NULL);
    initialize_board();

    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
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

    printf("Server started, waiting for players to connect...\n");

    pthread_create(&tid, NULL, handle_client, &server_socket);
    pthread_join(tid, NULL);

    close(server_socket);
    pthread_mutex_destroy(&lock);
    return 0;
}

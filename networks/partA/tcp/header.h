#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 12345
#define BUFFER_SIZE 1024
#define BOARD_SIZE 3
#define EMPTY ' '
#define PLAYER1 'X'
#define PLAYER2 'O'

typedef struct {
    int socket;
    char symbol;
} Player;
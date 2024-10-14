#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>

#define PORT 8080
#define DATA_SIZE 100
#define MAX_CHUNKS 10
#define TIMEOUT_USEC 100000

typedef struct {
    int seq_num;
    int total_chunks;
    char data[DATA_SIZE];
} Packet;

typedef struct {
    Packet packet;
    struct timeval send_time;
    int acked;
} PacketInfo;
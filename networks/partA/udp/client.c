#include "header.h"


int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int row, col;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // send message to server, registers the player
    sendto(sockfd, "Hello", 5, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    while (1) {
        memset(buffer, 0, sizeof(buffer)); // clear buffer
        socklen_t addr_len = sizeof(server_addr);
        int received = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, &addr_len);
        if (received <= 0) {
            printf("Server disconnected.\n");
            break;
        }
        buffer[received] = '\0';
        printf("%s", buffer);

        if (strstr(buffer, "Your turn")) {
            printf("Enter your move (row col): ");
            scanf("%d %d", &row, &col);
            snprintf(buffer, sizeof(buffer), "%d %d", row, col);
            sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        }

        if (strstr(buffer, "Do you want to play again?")) {
            char choice[10];
            scanf("%s", choice);
            snprintf(buffer, sizeof(buffer), "%s", choice);
            sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        }
    }

    close(sockfd);
    return 0;
}

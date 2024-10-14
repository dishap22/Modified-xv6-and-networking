#include "header.h"

int main() {
    // initialization stuff
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int row, col;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to the server failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        memset(buffer, 0, sizeof(buffer)); // clear out  buffer of any prev data
        int received = recv(sockfd, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            printf("Server disconnected.\n");
            break;
        } // if we don't get data from the server it has disconnected
        buffer[received] = '\0';  
        printf("%s", buffer); // print state of game

        if (strstr(buffer, "Your turn")) {
            printf("Enter your move (row col): ");
            scanf("%d %d", &row, &col);
            snprintf(buffer, sizeof(buffer), "%d %d\n", row, col);
            send(sockfd, buffer, strlen(buffer), 0); // send move to server
        }

        if (strstr(buffer, "Do you want to play again?")) {
            char choice[10];
            scanf("%s", choice);
            snprintf(buffer, sizeof(buffer), "%s\n", choice);
            send(sockfd, buffer, strlen(buffer), 0); // send play again or not to server
        }
    }

    close(sockfd);
    return 0;
}

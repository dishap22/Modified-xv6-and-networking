#include "header.h"

int main() {
    // Initialization code
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    Packet packet;
    char ack[sizeof(Packet)];
    char* chunks[MAX_CHUNKS] = {NULL};  // Array to store each chunk
    int total_chunks = 0;
    int chunks_received = 0;

    //Created socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    //Bind server address, port
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server is running on port %d\n", PORT);

    while (1) {
        int n = recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, &addr_len);
        // if packet received successfully
        if (n > 0) {
            printf("Received chunk %d: %s\n", packet.seq_num, packet.data);
            
            // Skip every third ACK
            /*if ((packet.seq_num + 1) % 3 != 0) { // Skip every third ACK
                snprintf(ack, sizeof(ack), "ACK %d", packet.seq_num);
                sendto(sockfd, ack, strlen(ack), 0, (struct sockaddr *)&client_addr, addr_len);
                printf("Sent ACK for chunk %d\n", packet.seq_num);
            } else {
                printf("Skipped ACK for chunk %d\n", packet.seq_num);
                continue;
            }*/
            
            // Send ACK
            snprintf(ack, sizeof(ack), "ACK %d", packet.seq_num);
            sendto(sockfd, ack, strlen(ack), 0, (struct sockaddr *)&client_addr, addr_len);
            printf("Sent ACK for chunk %d\n", packet.seq_num);
            
            // Stores chunk we received so it can reassemble message
            if (packet.seq_num == 0) {
                total_chunks = packet.total_chunks; 
                chunks_received = 0;

                for (int i = 0; i < MAX_CHUNKS; i++) {
                    chunks[i] = NULL; // Ensure each pointer starts as NULL
                }
            }

            if (chunks[packet.seq_num] == NULL) {  // Only store if not already stored
                chunks[packet.seq_num] = malloc(DATA_SIZE); 
                if (chunks[packet.seq_num] != NULL) { 
                    strncpy(chunks[packet.seq_num], packet.data, DATA_SIZE);
                    chunks_received++;
                } else {
                    perror("Failed to allocate memory for chunk");
                }
            }

            // if we have received all chunks, print the message
            if (chunks_received == total_chunks) {
                printf("Received complete message: ");
                for (int i = 0; i < total_chunks; i++) {
                    if (chunks[i] != NULL) {
                        printf("%s", chunks[i]);
                        free(chunks[i]); 
                    }
                }
                printf("\n");

                total_chunks = 0;
                chunks_received = 0;
            }
        }
    }

    close(sockfd);
    return 0;
}

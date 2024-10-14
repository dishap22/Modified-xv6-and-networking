#include "header.h"

void set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl get failed");
        exit(EXIT_FAILURE);
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl set non-blocking failed");
        exit(EXIT_FAILURE);
    }
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    PacketInfo packet_info[MAX_CHUNKS];
    char ack[sizeof(Packet)];
    char message[DATA_SIZE * MAX_CHUNKS - 10];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    set_nonblocking(sockfd);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    while (1) {
        printf("Enter message to send (or type 'exit' to quit): ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = 0; 

        if (strcmp(message, "exit") == 0) {
            break;
        }

        int total_chunks = (strlen(message) / (DATA_SIZE-1)) + 1;

        // initialize packet info
        for (int i = 0; i < total_chunks; i++) {
            packet_info[i].packet.seq_num = i; 
            packet_info[i].packet.total_chunks = total_chunks; 
            strncpy(packet_info[i].packet.data, message + (i * (DATA_SIZE-1)), DATA_SIZE - 1); 
            packet_info[i].packet.data[DATA_SIZE - 1] = '\0';
            packet_info[i].acked = 0;
            gettimeofday(&packet_info[i].send_time, NULL); 
        }

        // send packets
        struct timeval current_time;
        int all_acked = 0;
        while (!all_acked) {
            all_acked = 1; // assume all acked by default and update this later if not
            for (int i = 0; i < total_chunks; i++) {
                if (!packet_info[i].acked) {
                    all_acked = 0;
                    gettimeofday(&current_time, NULL);
                    // if time is beyond resend time or we've never sent packet before, send it
                    if (timercmp(&current_time, &packet_info[i].send_time, >) ||
                        (packet_info[i].send_time.tv_sec == 0 && packet_info[i].send_time.tv_usec == 0)) {
                        sendto(sockfd, &packet_info[i].packet, sizeof(packet_info[i].packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                        printf("Sent chunk %d: %s\n", packet_info[i].packet.seq_num, packet_info[i].packet.data);
                        gettimeofday(&packet_info[i].send_time, NULL);
                        struct timeval timeout = {0, TIMEOUT_USEC};
                        timeradd(&packet_info[i].send_time, &timeout, &packet_info[i].send_time); // add time and decide when next to send
                    }
                }
            }

            // check for ACKs
            int n = recvfrom(sockfd, ack, sizeof(ack), 0, (struct sockaddr *)&server_addr, &addr_len);
            if (n > 0) {
                int ack_seq_num;
                ack[n] = '\0';
                sscanf(ack, "ACK %d", &ack_seq_num);
                if (ack_seq_num >= 0 && ack_seq_num < total_chunks) {
                    packet_info[ack_seq_num].acked = 1;
                    printf("Received ACK for chunk %d\n", ack_seq_num);
                }
            }
        }
    }

    close(sockfd);
    return 0;
}

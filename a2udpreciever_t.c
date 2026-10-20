#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9091

int main(void)
{
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    char buffer[1024];
    int drop_first_ack = 1;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(sockfd);
        return 1;
    }

    printf("UDP receiver listening on port %d...\n", PORT);

    while (1)
    {
        int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                         (struct sockaddr *)&client_addr, &client_len);

        if (n < 0)
        {
            perror("recvfrom");
            continue;
        }

        buffer[n] = '\0';
        printf("Received: %s\n", buffer);

        /* Simulate losing the first ACK */
        if (drop_first_ack)
        {
            printf("Simulating lost ACK: not sending ACK this time.\n");
            drop_first_ack = 0;
            continue;
        }

        const char *ack = "ACK=1";

        sendto(sockfd, ack, strlen(ack), 0,
               (struct sockaddr *)&client_addr, client_len);

        printf("Sent: %s\n", ack);
    }

    close(sockfd);
    return 0;
}
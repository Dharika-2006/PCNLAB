#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define PORT 9091
#define MAX_RETRIES 5

int main(void)
{
    int sockfd;
    struct sockaddr_in receiver_addr;
    struct timeval timeout;

    char message[] = "SEQ=1 DATA=Hello";
    char buffer[1024];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        return 1;
    }

    /* Set a 2-second receive timeout for ACKs */
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
                   &timeout, sizeof(timeout)) < 0)
    {
        perror("setsockopt");
        close(sockfd);
        return 1;
    }

    memset(&receiver_addr, 0, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(PORT);
    receiver_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    for (int attempt = 1; attempt <= MAX_RETRIES; attempt++)
    {
        printf("\nAttempt %d: sending packet...\n", attempt);

        sendto(sockfd, message, strlen(message), 0,
               (struct sockaddr *)&receiver_addr,
               sizeof(receiver_addr));

        printf("Sent: %s\n", message);
        printf("Waiting for ACK (2 seconds)...\n");

        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);

        int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                         (struct sockaddr *)&from_addr, &from_len);

        if (n >= 0)
        {
            buffer[n] = '\0';

            if (strcmp(buffer, "ACK=1") == 0)
            {
                printf("Received: %s\n", buffer);
                printf("ACK received. Transmission successful!\n");
                close(sockfd);
                return 0;
            }

            printf("Received unexpected reply: %s\n", buffer);
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            printf("Timeout! No ACK received.\n");
            printf("Retransmitting packet...\n");
        }
        else
        {
            perror("recvfrom");
        }
    }

    printf("\nFailed: maximum retransmissions reached.\n");
    close(sockfd);
    return 1;
}






















#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server = {0}, client;
    socklen_t len = sizeof(client);
    char msg[100];

    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr = INADDR_ANY;
    bind(s, (struct sockaddr *)&server, sizeof(server));

    while (1) {
        int n = recvfrom(s, msg, sizeof(msg)-1, 0,
                         (struct sockaddr *)&client, &len);
        msg[n] = '\0';
        printf("Received: %s\n", msg);

        sendto(s, msg, n, 0, (struct sockaddr *)&client, len);
    }
}

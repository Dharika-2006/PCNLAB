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























#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

int main() {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server = {0};
    char msg[100], reply[100];
    socklen_t len = sizeof(server);

    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    struct timeval timer = {2, 0};
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timer, sizeof(timer));

    printf("Enter message: ");
    fgets(msg, sizeof(msg), stdin);
    msg[strcspn(msg, "\n")] = '\0';

    for (int i = 1; i <= 3; i++) {
        sendto(s, msg, strlen(msg), 0,
               (struct sockaddr *)&server, len);
        printf("Sent (attempt %d)\n", i);

        int n = recvfrom(s, reply, sizeof(reply)-1, 0,
                         (struct sockaddr *)&server, &len);

        if (n >= 0) {
            reply[n] = '\0';
            printf("Echo from server: %s\n", reply);
            close(s);
            return 0;
        }

        printf("Timeout! Retrying...\n");
    }

    printf("No reply after 3 attempts.\n");
    close(s);
    return 0;
}

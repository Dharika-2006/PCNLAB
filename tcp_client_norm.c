#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

int main()
{
    int sock;
    struct sockaddr_in server;
    char buffer[1024];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    if (connect(sock, (struct sockaddr *)&server,
                sizeof(server)) < 0)
    {
        perror("connect");
        close(sock);
        return 1;
    }

    int seq = 1;

    while (1)
    {
        printf("Enter message (exit to quit): ");

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
            break;

        if (strncmp(buffer, "exit", 4) == 0)
            break;

        int ack_received = 0;

        for (int attempt = 1; attempt <= 3; attempt++)
        {
            if (send(sock, buffer, strlen(buffer), 0) < 0)
            {
                perror("send");
                goto cleanup;
            }

            printf("Packet %d sent (attempt %d)\n", seq, attempt);

            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(sock, &readfds);

            struct timeval timer;
            timer.tv_sec = 3;
            timer.tv_usec = 0;

            int ready = select(sock + 1, &readfds,
                               NULL, NULL, &timer);

            if (ready > 0 && FD_ISSET(sock, &readfds))
            {
                int n = recv(sock, buffer, sizeof(buffer) - 1, 0);

                if (n <= 0)
                {
                    printf("Server disconnected.\n");
                    goto cleanup;
                }

                buffer[n] = '\0';
                printf("ACK received: %s", buffer);
                ack_received = 1;
                break;
            }
            else if (ready == 0)
            {
                printf("Timer expired! Retrying...\n");
            }
            else
            {
                perror("select");
                goto cleanup;
            }
        }

        if (!ack_received)
            printf("Packet %d failed after 3 attempts\n", seq);

        seq++;
    }

cleanup:
    close(sock);
    return 0;
}
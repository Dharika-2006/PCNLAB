#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main()
{
    int server_fd, client_fd;
    struct sockaddr_in server, client;
    socklen_t len = sizeof(client);
    char buffer[1024];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&server,
             sizeof(server)) < 0)
    {
        perror("bind");
        close(server_fd);
        return 1;
    }

    listen(server_fd, 5);
    printf("TCP Server waiting...\n");

    client_fd = accept(server_fd, (struct sockaddr *)&client, &len);
    if (client_fd < 0)
    {
        perror("accept");
        close(server_fd);
        return 1;
    }

    while (1)
    {
        int n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (n <= 0)
            break;

        buffer[n] = '\0';
        printf("Received: %s", buffer);

        if (send(client_fd, buffer, n, 0) < 0)
        {
            perror("send");
            break;
        }
    }

    close(client_fd);
    close(server_fd);
    return 0;
}
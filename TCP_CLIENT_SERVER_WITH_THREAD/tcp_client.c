#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

int main()
{
    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = {0};
    struct timeval tv = {5, 0}; // 5 sec recv timeout
    struct timespec t1, t2;
    char user[50], msg[100], buf[200], reply[300];

    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    a.sin_family = AF_INET;
    a.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);
    connect(s, (struct sockaddr *)&a, sizeof(a));

    printf("Username: ");
    scanf("%49s", user);
    getchar();
    printf("Enter message: ");
    fgets(msg, sizeof(msg), stdin);
    msg[strcspn(msg, "\n")] = 0;

    snprintf(buf, sizeof(buf), "%s %s", user, msg);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    send(s, buf, strlen(buf), 0);

    int n = recv(s, reply, sizeof(reply) - 1, 0);
    clock_gettime(CLOCK_MONOTONIC, &t2);
    if (n <= 0)
    {
        printf("Timeout / no reply\n");
        return 1;
    }
    reply[n] = '\0';

    printf("Echo from server: %s\n", reply);
    printf("RTT: %.3f ms\n", (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_nsec - t1.tv_nsec) / 1e6);
    close(s);
}
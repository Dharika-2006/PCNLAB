#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

int main()
{
    int s = socket(AF_INET, SOCK_DGRAM, 0), n = 0, i;
    struct sockaddr_in a = {0};
    struct timeval tv = {2, 0}; // 2 sec ACK timeout
    struct timespec t1, t2;
    char user[50], msg[100], buf[200], reply[300];

    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    a.sin_family = AF_INET;
    a.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);

    printf("Username: ");
    scanf("%49s", user);
    getchar();
    printf("Enter message: ");
    fgets(msg, sizeof(msg), stdin);
    msg[strcspn(msg, "\n")] = 0;
    snprintf(buf, sizeof(buf), "%s %s", user, msg);

    for (i = 1; i <= 3; i++)
    { // retransmit up to 3 times
        clock_gettime(CLOCK_MONOTONIC, &t1);
        sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&a, sizeof(a));
        n = recvfrom(s, reply, sizeof(reply) - 1, 0, NULL, NULL);
        clock_gettime(CLOCK_MONOTONIC, &t2);
        if (n > 0)
            break;
        printf("Timeout, retry %d/3\n", i);
    }
    if (n <= 0)
    {
        printf("No ACK received\n");
        return 1;
    }

    reply[n] = '\0';
    printf("%s\n", reply);
    printf("RTT: %.3f ms\n", (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_nsec - t1.tv_nsec) / 1e6);
    close(s);
}
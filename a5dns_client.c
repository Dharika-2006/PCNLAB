#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#define PORT 5300
#define N 5

struct
{
    char name[256], ip[64];
} cache[N];
int cnt;

int main()
{
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    struct timeval tv = {2, 0};
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    struct sockaddr_in a = {AF_INET, htons(PORT)};
    a.sin_addr.s_addr = inet_addr("127.0.0.1");
    char d[256], r[64];
    while (1)
    {
        printf("Domain (exit to quit): ");
        if (!fgets(d, sizeof d, stdin))
            break;
        d[strcspn(d, "\r\n")] = 0;
        if (!strcmp(d, "exit"))
            break;

        int hit = 0;
        for (int i = 0; i < N && i < cnt; i++)
            if (!strcmp(cache[i].name, d))
            {
                printf("[CACHE] %s\n", cache[i].ip);
                hit = 1;
            }
        if (hit)
            continue;

        sendto(s, d, strlen(d), 0, (void *)&a, sizeof a);
        int n = recvfrom(s, r, sizeof r - 1, 0, NULL, NULL);
        if (n < 0)
        {
            puts("Request timed out");
            continue;
        }
        r[n] = 0;
        printf("%s\n", r);
        if (r[0] >= '0' && r[0] <= '9')
        {
            int k = cnt++ % N;
            strcpy(cache[k].name, d);
            strcpy(cache[k].ip, r);
        }
    }
}
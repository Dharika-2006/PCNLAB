#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#define PORT 8080

int main()
{
    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = {AF_INET, htons(PORT)};
    a.sin_addr.s_addr = inet_addr("127.0.0.1");
    connect(s, (void *)&a, sizeof a);
    struct timeval tv = {2, 0}; /* client timer: 2 s */
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    char roll[64], buf[256] = {0};
    int n = 0;
    printf("Enter Roll Number: ");
    scanf("%63s", roll);
    for (int t = 0; t < 3; t++)
    { /* send request, wait, retransmit on timeout */
        write(s, roll, strlen(roll));
        n = read(s, buf, sizeof buf - 1);
        if (n > 0)
        {
            buf[n] = 0;
            printf("%s\n", buf);
            write(s, "ACK", 3);
            break;
        }
        if (n == 0)
            break;
        puts("Timeout, retransmitting request");
    }
    if (n <= 0)
        puts("No response from server");
    close(s);
}

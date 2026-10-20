#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define PORT 8081

int main()
{
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in a = {AF_INET, htons(PORT)};
    a.sin_addr.s_addr = inet_addr("127.0.0.1");
    char roll[64], buf[256] = {0};
    printf("Enter Roll Number: ");
    scanf("%63s", roll);
    sendto(s, roll, strlen(roll), 0, (void *)&a, sizeof a);
    int n = recvfrom(s, buf, sizeof buf - 1, 0, NULL, NULL);
    if (n > 0)
        buf[n] = '\0';
    printf("%s\n", buf);
}
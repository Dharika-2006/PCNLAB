#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#define PORT 8080

int main()
{
    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = {AF_INET, htons(PORT)};
    a.sin_addr.s_addr = inet_addr("127.0.0.1");
    connect(s, (void *)&a, sizeof a);
    char roll[64], buf[256] = {0};
    printf("Enter Roll Number: ");
    scanf("%63s", roll);
    write(s, roll, strlen(roll));
    read(s, buf, sizeof buf - 1);
    printf("%s\n", buf);
    close(s);
}
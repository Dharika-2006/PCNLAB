#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

int main()
{
    int s = socket(AF_INET, SOCK_DGRAM, 0), n;
    struct sockaddr_in a = {0}, c;
    socklen_t cl = sizeof(c);
    char buf[200], user[50], text[150], out[300], ts[30];

    a.sin_family = AF_INET;
    a.sin_port = htons(8080);
    a.sin_addr.s_addr = INADDR_ANY;
    bind(s, (struct sockaddr *)&a, sizeof(a));
    printf("UDP server listening on 8080...\n");

    while ((n = recvfrom(s, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&c, &cl)) > 0)
    {
        buf[n] = '\0';
        text[0] = '\0';
        sscanf(buf, "%49s %149[^\n]", user, text);
        time_t t = time(NULL);
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&t));
        printf("[%s] %s: %s\n", ts, user, text);
        snprintf(out, sizeof(out), "ACK [%s] %s: %s", ts, user, text);
        sendto(s, out, strlen(out), 0, (struct sockaddr *)&c, cl);
    }
}
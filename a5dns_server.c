#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define PORT 5300

struct
{
    char *name, *ip;
} db[] = {
    {"google.com", "142.250.183.14"},
    {"example.com", "93.184.216.34"},
    {"ssn.edu.in", "10.0.0.5"},
    {"localhost", "127.0.0.1"}};

int valid(char *s)
{
    if (!*s)
        return 0;
    for (; *s; s++)
        if (!isalnum((unsigned char)*s) && *s != '.' && *s != '-')
            return 0;
    return 1;
}

char *resolve(char *name)
{
    for (int i = 0; i < 4; i++)
        if (!strcmp(db[i].name, name))
            return db[i].ip;
    return "NXDOMAIN";
}

int main()
{
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in a = {AF_INET, htons(PORT), {INADDR_ANY}}, c;
    socklen_t l = sizeof c;
    bind(s, (void *)&a, sizeof a);
    char q[256];
    while (1)
    {
        int n = recvfrom(s, q, sizeof q - 1, 0, (void *)&c, &l);
        if (n < 0)
            continue;
        q[n] = 0;
        q[strcspn(q, "\r\n")] = 0;
        if (!strcmp(q, "slow.com"))
            continue; /* simulate timeout: no reply */
        char *r = valid(q) ? resolve(q) : "INVALID FORMAT";
        sendto(s, r, strlen(r), 0, (void *)&c, l);
    }
}
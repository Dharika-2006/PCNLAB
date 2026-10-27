#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
void find(char *t, char *out)
{
    char l[300], *c;
    FILE *f = fopen("problemss.csv", "r");
    strcpy(out, "No solution found");
    if (!f)
    {
        strcpy(out, "CSV file missing");
        return;
    }
    while (fgets(l, sizeof(l), f))
    {
        l[strcspn(l, "\r\n")] = 0;
        if (!(c = strchr(l, ',')))
            continue;
        *c = 0;
        if (strcasestr(t, l))
        {
            strcpy(out, c + 1);
            break;
        }
    }
    fclose(f);
}
int main()
{
    int s = socket(AF_INET, SOCK_DGRAM, 0), n;
    struct sockaddr_in a = {0}, c;
    socklen_t l = sizeof(c);
    char b[200], u[50], t[150], r[300], o[600], ts[30];
    a.sin_family = AF_INET;
    a.sin_port = htons(8080);
    a.sin_addr.s_addr = INADDR_ANY;
    bind(s, (struct sockaddr *)&a, sizeof(a));
    printf("UDP support server on 8080...\n");
    while ((n = recvfrom(s, b, sizeof(b) - 1, 0, (struct sockaddr *)&c, &l)) > 0)
    {
        b[n] = 0;
        t[0] = 0;
        sscanf(b, "%49s %149[^\n]", u, t);
        find(t, r);
        time_t now = time(NULL);
        strftime(ts, sizeof(ts), "%H:%M:%S", localtime(&now));
        snprintf(o, sizeof(o), "[%s] %s: %s\n  Solution: %s", ts, u, t, r);
        printf("%s\n", o);
        sendto(s, "ACK", 3, 0, (struct sockaddr *)&c, l);     // acknowledgement
        sendto(s, o, strlen(o), 0, (struct sockaddr *)&c, l); // solution
    }
}
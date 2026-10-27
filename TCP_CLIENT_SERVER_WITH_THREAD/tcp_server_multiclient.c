#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>
void find(char *t, char *out)
{ // search CSV for a matching problem
    char l[300], *c;
    FILE *f = fopen("problems.csv", "r");
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
void *h(void *p)
{
    int c = (int)(long)p, n;
    char b[200], u[50], t[150], s[300], o[600], ts[30];
    struct timeval tv = {60, 0}; // 60 sec idle timer
    setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    while ((n = recv(c, b, sizeof(b) - 1, 0)) > 0)
    {
        b[n] = 0;
        t[0] = 0;
        sscanf(b, "%49s %149[^\n]", u, t);
        find(t, s);
        time_t now = time(NULL);
        strftime(ts, sizeof(ts), "%H:%M:%S", localtime(&now));
        snprintf(o, sizeof(o), "[%s] %s: %s\n  Solution: %s\n", ts, u, t, s);
        printf("%s", o);
        send(c, o, strlen(o), 0); // reply only to the asking client
    }
    close(c);
    return NULL;
}
int main()
{
    int s = socket(AF_INET, SOCK_STREAM, 0), c, o = 1;
    struct sockaddr_in a = {0};
    pthread_t th;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &o, sizeof(o));
    a.sin_family = AF_INET;
    a.sin_port = htons(8080);
    a.sin_addr.s_addr = INADDR_ANY;
    bind(s, (struct sockaddr *)&a, sizeof(a));
    listen(s, 5);
    printf("Support server on 8080...\n");
    while ((c = accept(s, NULL, NULL)) >= 0)
    {
        pthread_create(&th, NULL, h, (void *)(long)c);
        pthread_detach(th);
    }
}
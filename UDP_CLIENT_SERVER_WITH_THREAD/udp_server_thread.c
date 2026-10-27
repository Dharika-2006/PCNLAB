#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>

struct job
{
    int s;
    struct sockaddr_in c;
    char buf[200];
};

void *handle(void *p)
{
    struct job *j = p;
    char user[50], text[150], out[300], ts[30];

    text[0] = '\0';
    sscanf(j->buf, "%49s %149[^\n]", user, text);
    time_t t = time(NULL);
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&t));
    printf("[%s] %s: %s\n", ts, user, text);
    snprintf(out, sizeof(out), "ACK [%s] %s: %s", ts, user, text);
    sendto(j->s, out, strlen(out), 0, (struct sockaddr *)&j->c, sizeof(j->c));
    free(j);
    return NULL;
}

int main()
{
    int s = socket(AF_INET, SOCK_DGRAM, 0), n;
    struct sockaddr_in a = {0};
    pthread_t th;

    a.sin_family = AF_INET;
    a.sin_port = htons(8080);
    a.sin_addr.s_addr = INADDR_ANY;
    bind(s, (struct sockaddr *)&a, sizeof(a));
    printf("MT UDP server listening on 8080...\n");

    while (1)
    {
        struct job *j = malloc(sizeof(*j));
        socklen_t cl = sizeof(j->c);
        n = recvfrom(s, j->buf, sizeof(j->buf) - 1, 0, (struct sockaddr *)&j->c, &cl);
        if (n <= 0)
        {
            free(j);
            continue;
        }
        j->buf[n] = '\0';
        j->s = s;
        pthread_create(&th, NULL, handle, j);
        pthread_detach(th);
    }
}
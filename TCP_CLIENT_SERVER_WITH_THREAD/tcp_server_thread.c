#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>

void *handle(void *p)
{
    int c = (int)(long)p, n;
    char buf[200], user[50], text[150], out[300], ts[30];

    while ((n = recv(c, buf, sizeof(buf) - 1, 0)) > 0)
    {
        buf[n] = '\0';
        sscanf(buf, "%49s %149[^\n]", user, text);
        time_t t = time(NULL);
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&t));
        snprintf(out, sizeof(out), "[%s] %s: %s", ts, user, text);
        printf("%s\n", out);
        send(c, out, strlen(out), 0);
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
    printf("MT server listening on 8080...\n");

    while ((c = accept(s, NULL, NULL)) >= 0)
    {
        pthread_create(&th, NULL, handle, (void *)(long)c);
        pthread_detach(th);
    }
}
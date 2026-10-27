#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

int main()
{
    int s = socket(AF_INET, SOCK_STREAM, 0), c, o = 1;
    struct sockaddr_in a = {0};
    char buf[200], user[50], text[150], out[300], ts[30];

    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &o, sizeof(o));
    a.sin_family = AF_INET;
    a.sin_port = htons(8080);
    a.sin_addr.s_addr = INADDR_ANY;
    bind(s, (struct sockaddr *)&a, sizeof(a));
    listen(s, 5);
    printf("Server listening on 8080...\n");

    while ((c = accept(s, NULL, NULL)) >= 0)
    {
        int n = recv(c, buf, sizeof(buf) - 1, 0);
        if (n > 0)
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
    }
}
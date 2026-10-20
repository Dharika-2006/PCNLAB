#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

int main()
{
    char host[256], path[256] = "/", buf[4096];
    printf("Enter URL (example.com/file.txt): ");
    scanf("%255s", host);

    char *p = strchr(host, '/');
    if (p)
    {
        strcpy(path, p);
        *p = '\0';
    }

    int s = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port = htons(80);
    a.sin_addr.s_addr = inet_addr(host);

    if (connect(s, (struct sockaddr *)&a, sizeof a) < 0)
    {
        perror("Connect failed");
        return 1;
    }

    char req[512];
    sprintf(req, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", path, host);

    clock_t start = clock();
    send(s, req, strlen(req), 0);

    int n;
    FILE *f = fopen("download.html", "wb");
    while ((n = recv(s, buf, sizeof buf, 0)) > 0)
        fwrite(buf, 1, n, f);

    fclose(f);
    close(s);

    double t = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("Saved: download.html\n");
    printf("Time: %.2f s\n", t);

    return 0;
}

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <sys/socket.h>

double now()
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

int main()
{
    char url[512], host[256], port[8] = "80", path[256] = "/", hp[256], req[1024], buf[4096], fn[300];
    printf("Enter URL: ");
    scanf("%511s", url);

    char *p = strstr(url, "://");
    p = p ? p + 3 : url;
    char *sl = strchr(p, '/');
    if (sl)
        strcpy(path, sl);
    int hl = sl ? sl - p : strlen(p);
    strncpy(hp, p, hl);
    hp[hl] = 0;
    char *c = strchr(hp, ':');
    if (c)
    {
        *c = 0;
        strcpy(port, c + 1);
    }
    strcpy(host, hp);
    char *name = strrchr(path, '/') + 1;
    sprintf(fn, "downloaded_%s", *name ? name : "index.html");
    printf("Host: %s\nPort: %s\nPath: %s\n", host, port, path);

    struct addrinfo h = {0}, *r;
    h.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, port, &h, &r))
    {
        puts("DNS failed");
        return 1;
    }
    int s = socket(r->ai_family, r->ai_socktype, 0);
    if (connect(s, r->ai_addr, r->ai_addrlen) < 0)
    {
        puts("Connect failed");
        return 1;
    }

    sprintf(req, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
    double t0 = now(), t1 = 0;
    send(s, req, strlen(req), 0);

    char *data = NULL;
    long total = 0;
    int n;
    while ((n = recv(s, buf, sizeof buf, 0)) > 0)
    {
        if (!t1)
            t1 = now();
        data = realloc(data, total + n);
        memcpy(data + total, buf, n);
        total += n;
    }
    double t2 = now();
    close(s);
    if (!data)
    {
        puts("No response");
        return 1;
    }

    char *body = memmem(data, total, "\r\n\r\n", 4);
    if (!body)
    {
        puts("Bad response");
        return 1;
    }
    body += 4;
    long blen = total - (body - data);
    FILE *f = fopen(fn, "wb");
    fwrite(body, 1, blen, f);
    fclose(f);

    printf("Saved: %s\n", fn);
    printf("Response time: %.4f s\n", t1 - t0);
    printf("Download time: %.4f s\n", t2 - t1);
    printf("Data size: %ld bytes\n", blen);
    printf("Throughput: %.2f KB/s\n", blen / 1024.0 / (t2 - t0));
}
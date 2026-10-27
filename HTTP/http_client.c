#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <sys/socket.h>
static double now()
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}
int main()
{
    char url[512], host[256], port[8] = "80", path[512] = "/", req[1024], tmp[4096], name[600], *buf = NULL;
    printf("Enter URL: ");
    scanf("%511s", url);
    char *p = strstr(url, "://");
    p = p ? p + 3 : url; /* skip http:// */
    char *s = strchr(p, '/');
    if (s)
        snprintf(path, sizeof path, "%s", s);   /* path */
    sscanf(p, "%255[^:/]:%7[0-9]", host, port); /* host[:port] */
    struct addrinfo h = {0}, *r;
    h.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, port, &h, &r))
        return puts("DNS failed");
    int fd = socket(r->ai_family, SOCK_STREAM, 0);
    double t0 = now();
    if (connect(fd, r->ai_addr, r->ai_addrlen))
        return puts("Connect failed");
    double t1 = now(); /* TCP connected */
    snprintf(req, sizeof req, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
    double t2 = now();
    send(fd, req, strlen(req), 0); /* request sent */
    size_t n = 0;
    ssize_t k;
    double t3 = 0;
    while ((k = recv(fd, tmp, sizeof tmp, 0)) > 0)
    {
        if (!n)
            t3 = now();
        buf = realloc(buf, n + k);
        memcpy(buf + n, tmp, k);
        n += k;
    }
    double t4 = now();
    close(fd); /* last byte received */
    char *b = n ? memmem(buf, n, "\r\n\r\n", 4) : NULL;
    if (!b)
        return puts("Bad response");
    b += 4;
    size_t sz = n - (b - buf); /* body size */
    char *f = strrchr(path, '/') + 1;
    snprintf(name, sizeof name, "downloaded_%s", *f ? f : "index.html");
    FILE *o = fopen(name, "wb");
    fwrite(b, 1, sz, o);
    fclose(o);
    printf("%.*s\nSaved as: %s\n", (int)strcspn(buf, "\r"), buf, name);
    printf("TCP connect time : %.3f ms\nResponse time    : %.3f ms (request -> first byte)\n", (t1 - t0) * 1e3, (t3 - t2) * 1e3);
    printf("Download time    : %.3f ms (request -> last byte)\nData size        : %zu bytes\nThroughput       : %.3f Mbps\n", (t4 - t2) * 1e3, sz, sz * 8 / (t4 - t2) / 1e6);
    return 0;
}
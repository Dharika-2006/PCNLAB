#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
static double now()
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}
int main()
{
    const char *host = "example.com";
    char req[256], buf[4096];
    struct addrinfo h = {0}, *r;
    h.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, "443", &h, &r))
        return puts("DNS failed");
    int fd = socket(r->ai_family, SOCK_STREAM, 0);
    double t0 = now();
    if (connect(fd, r->ai_addr, r->ai_addrlen))
        return puts("TCP connect failed");
    double t1 = now(); /* TCP done (~1 RTT) */
    SSL_CTX *c = SSL_CTX_new(TLS_client_method());
    SSL_CTX_set_default_verify_paths(c);
    SSL_CTX_set_verify(c, SSL_VERIFY_PEER, NULL); /* trust store + verify */
    SSL *s = SSL_new(c);
    SSL_set_fd(s, fd);
    SSL_set_tlsext_host_name(s, host);
    SSL_set1_host(s, host);
    if (SSL_connect(s) != 1)
        return puts("TLS handshake / certificate verification failed");
    double t2 = now(); /* handshake done */
    snprintf(req, sizeof req, "GET / HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", host);
    double t3 = now();
    SSL_write(s, req, strlen(req)); /* encrypted request */
    size_t n = 0;
    int k;
    double t4 = 0;
    while ((k = SSL_read(s, buf, sizeof buf)) > 0)
    {
        if (!n)
            t4 = now();
        n += k;
    } /* decrypted response */
    double t5 = now();
    printf("TLS version: %s | Cipher: %s | Cert verify: %s\n", SSL_get_version(s), SSL_get_cipher(s),
           X509_verify_cert_error_string(SSL_get_verify_result(s)));
    printf("TCP connection time : %.3f ms (~1 RTT)\nSSL handshake time  : %.3f ms\n", (t1 - t0) * 1e3, (t2 - t1) * 1e3);
    printf("Request RTT (TTFB)  : %.3f ms\nData received       : %zu bytes\nData transfer rate  : %.3f Mbps\n", (t4 - t3) * 1e3, n, n * 8 / (t5 - t3) / 1e6);
    SSL_shutdown(s);
    close(fd);
    return 0;
}

mkdir CN_Web_Server &&cd CN_Web_Server
#put index.html, notes.txt, sample.pdf, image.jpg here
        python3 -
    m http.server 8000

#in another terminal
    gcc http_client.c -
    o http_client
            ./
        http_client
            Enter URL : http: // localhost:8000/sample.pdf
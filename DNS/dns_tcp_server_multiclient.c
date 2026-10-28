#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

struct { char *h, *ip; } db[] = {
    {"google.com", "142.250.183.14"}, {"example.com", "93.184.216.34"},
    {"github.com", "140.82.112.3"},   {"college.edu", "10.0.0.5"}};
struct cli { int c; struct sockaddr_in a; };

int valid(char *q) {
    if (!strchr(q, '.')) return 0;
    for (; *q; q++) if (!isalnum(*q) && *q != '.' && *q != '-') return 0;
    return 1;
}

void resolve(char *q, char *r) {
    if (!valid(q)) { strcpy(r, "ERROR: invalid format"); return; }
    strcpy(r, "NXDOMAIN");
    for (int i = 0; i < 4; i++) if (!strcmp(q, db[i].h)) sprintf(r, "A %s", db[i].ip);
}

void *handle(void *p) {
    struct cli k = *(struct cli *)p; free(p);
    char q[64], r[64]; int n;
    while ((n = recv(k.c, q, sizeof(q) - 1, 0)) > 0) {
        q[n] = 0;
        printf("[%s:%d] Query: %s\n", inet_ntoa(k.a.sin_addr), ntohs(k.a.sin_port), q);
        if (!strcmp(q, "timeout.test")) sleep(3);      // slow reply, only blocks this thread
        resolve(q, r);
        send(k.c, r, strlen(r), 0);
    }
    close(k.c); return NULL;
}

int main() {
    int s = socket(AF_INET, SOCK_STREAM, 0), o = 1;
    struct sockaddr_in a = {0};
    socklen_t l = sizeof(a);
    pthread_t th;

    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &o, sizeof(o));
    a.sin_family = AF_INET; a.sin_port = htons(8080); a.sin_addr.s_addr = INADDR_ANY;
    bind(s, (struct sockaddr *)&a, sizeof(a));
    listen(s, 10);
    printf("MT DNS TCP server on 8080...\n");

    while (1) {
        struct cli *k = malloc(sizeof(*k));
        k->c = accept(s, (struct sockaddr *)&k->a, &l);
        if (k->c < 0) { free(k); continue; }
        pthread_create(&th, NULL, handle, k);
        pthread_detach(th);
    }
}

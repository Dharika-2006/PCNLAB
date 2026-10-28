#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <arpa/inet.h>

struct { char *h, *ip; } db[] = {
    {"google.com", "142.250.183.14"}, {"example.com", "93.184.216.34"},
    {"github.com", "140.82.112.3"},   {"college.edu", "10.0.0.5"}};
struct job { int s; struct sockaddr_in c; char q[64]; };

int valid(char *q) {
    if (!strchr(q, '.')) return 0;
    for (; *q; q++) if (!isalnum(*q) && *q != '.' && *q != '-') return 0;
    return 1;
}

void *handle(void *p) {
    struct job *j = p; char r[64];
    printf("[%s:%d] Query: %s\n", inet_ntoa(j->c.sin_addr), ntohs(j->c.sin_port), j->q);
    if (!strcmp(j->q, "timeout.test")) { free(j); return NULL; }   // simulate dropped query
    if (!valid(j->q)) strcpy(r, "ERROR: invalid format");
    else {
        strcpy(r, "NXDOMAIN");
        for (int i = 0; i < 4; i++) if (!strcmp(j->q, db[i].h)) sprintf(r, "A %s", db[i].ip);
    }
    sendto(j->s, r, strlen(r), 0, (struct sockaddr *)&j->c, sizeof(j->c));
    free(j); return NULL;
}

int main() {
    int s = socket(AF_INET, SOCK_DGRAM, 0), n;
    struct sockaddr_in a = {0};
    pthread_t th;

    a.sin_family = AF_INET; a.sin_port = htons(8080); a.sin_addr.s_addr = INADDR_ANY;
    bind(s, (struct sockaddr *)&a, sizeof(a));
    printf("MT DNS UDP server on 8080...\n");

    while (1) {
        struct job *j = malloc(sizeof(*j));
        socklen_t l = sizeof(j->c);
        n = recvfrom(s, j->q, sizeof(j->q) - 1, 0, (struct sockaddr *)&j->c, &l);
        if (n <= 0) { free(j); continue; }
        j->q[n] = 0; j->s = s;
        pthread_create(&th, NULL, handle, j);
        pthread_detach(th);
    }
}

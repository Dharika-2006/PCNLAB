#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <arpa/inet.h>

struct
{
    char *h, *ip;
} db[] = {
    {"google.com", "142.250.183.14"}, {"example.com", "93.184.216.34"}, {"github.com", "140.82.112.3"}, {"college.edu", "10.0.0.5"}};

int tot, found, nx, bad, dropped, bin, bout;
double lk, lkmax; // lookup time in microseconds
struct timespec st;

double ms(struct timespec a, struct timespec b) { return (b.tv_sec - a.tv_sec) * 1000.0 + (b.tv_nsec - a.tv_nsec) / 1e6; }

int valid(char *q)
{
    if (!strchr(q, '.'))
        return 0;
    for (; *q; q++)
        if (!isalnum(*q) && *q != '.' && *q != '-')
            return 0;
    return 1;
}

void stats(int g)
{ // runs on Ctrl+C
    struct timespec e;
    clock_gettime(CLOCK_MONOTONIC, &e);
    double up = ms(st, e) / 1000;
    printf("\n===== SERVER METRICS =====\n");
    printf("Uptime               : %.1f s\n", up);
    printf("Queries received     : %d (%.2f queries/s)\n", tot, tot / up);
    printf("Resolved (A)         : %d (%.1f %%)\n", found, tot ? found * 100.0 / tot : 0);
    printf("NXDOMAIN             : %d (%.1f %%)\n", nx, tot ? nx * 100.0 / tot : 0);
    printf("Invalid format       : %d (%.1f %%)\n", bad, tot ? bad * 100.0 / tot : 0);
    printf("Dropped (simulated)  : %d\n", dropped);
    printf("Avg lookup time      : %.3f us\n", tot - dropped ? lk / (tot - dropped) : 0);
    printf("Max lookup time      : %.3f us\n", lkmax);
    printf("Bytes in / out       : %d / %d\n", bin, bout);
    exit(0);
}

int main()
{
    int s = socket(AF_INET, SOCK_DGRAM, 0), n, i;
    struct sockaddr_in a = {0}, c;
    socklen_t cl = sizeof(c);
    struct timespec t1, t2;
    char q[64], r[64];

    signal(SIGINT, stats);
    clock_gettime(CLOCK_MONOTONIC, &st);
    a.sin_family = AF_INET;
    a.sin_port = htons(8080);
    a.sin_addr.s_addr = INADDR_ANY;
    bind(s, (struct sockaddr *)&a, sizeof(a));
    printf("DNS UDP server on 8080... (Ctrl+C for metrics)\n");

    while ((n = recvfrom(s, q, sizeof(q) - 1, 0, (struct sockaddr *)&c, &cl)) > 0)
    {
        q[n] = 0;
        tot++;
        bin += n;
        printf("Query: %s\n", q);
        if (!strcmp(q, "timeout.test"))
        {
            dropped++;
            continue;
        } // simulate dropped query

        clock_gettime(CLOCK_MONOTONIC, &t1); // time only the lookup
        if (!valid(q))
        {
            strcpy(r, "ERROR: invalid format");
            bad++;
        }
        else
        {
            strcpy(r, "NXDOMAIN");
            for (i = 0; i < 4; i++)
                if (!strcmp(q, db[i].h))
                    sprintf(r, "A %s", db[i].ip);
            if (r[0] == 'A')
                found++;
            else
                nx++;
        }
        clock_gettime(CLOCK_MONOTONIC, &t2);
        double d = ms(t1, t2) * 1000;
        lk += d;
        if (d > lkmax)
            lkmax = d;

        sendto(s, r, strlen(r), 0, (struct sockaddr *)&c, cl);
        bout += strlen(r);
    }
}
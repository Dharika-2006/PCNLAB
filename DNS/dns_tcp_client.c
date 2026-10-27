#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

struct
{
    char h[64], ip[32];
} cache[5];
int cn;

double ms(struct timespec a, struct timespec b) { return (b.tv_sec - a.tv_sec) * 1000.0 + (b.tv_nsec - a.tv_nsec) / 1e6; }

int main()
{
    int s, i, n, tot = 0, hit = 0, net = 0, ok = 0, nx = 0, bad = 0, tmo = 0, fail = 0, sb = 0, rb = 0;
    double con = 0, rsp = 0, lat, sum = 0, mn = 1e9, mx = 0;
    struct sockaddr_in a = {0};
    struct timeval tv = {2, 0};
    struct timespec t0, t1, t2, g0, g1;
    char q[64], r[64];

    a.sin_family = AF_INET;
    a.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);
    clock_gettime(CLOCK_MONOTONIC, &g0);

    while (printf("Domain (exit to quit): "), scanf("%63s", q) == 1 && strcmp(q, "exit"))
    {
        tot++;
        for (i = 0; i < 5 && strcmp(cache[i].h, q); i++)
            ;
        if (i < 5)
        {
            hit++;
            printf("[CACHE] %s -> %s (0 network time)\n", q, cache[i].ip);
            continue;
        }

        s = socket(AF_INET, SOCK_STREAM, 0);
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        clock_gettime(CLOCK_MONOTONIC, &t0);
        if (connect(s, (struct sockaddr *)&a, sizeof(a)) < 0)
        {
            printf("Connection failed\n");
            fail++;
            close(s);
            continue;
        }
        clock_gettime(CLOCK_MONOTONIC, &t1); // handshake done
        send(s, q, strlen(q), 0);
        sb += strlen(q);
        n = recv(s, r, sizeof(r) - 1, 0);
        clock_gettime(CLOCK_MONOTONIC, &t2); // response received
        close(s);
        if (n <= 0)
        {
            printf("Timeout: no response from server (%.0f ms)\n", ms(t0, t2));
            tmo++;
            continue;
        }
        r[n] = 0;
        rb += n;
        net++;

        lat = ms(t0, t2);
        sum += lat;
        con += ms(t0, t1);
        rsp += ms(t1, t2);
        if (lat < mn)
            mn = lat;
        if (lat > mx)
            mx = lat;

        if (!strncmp(r, "A ", 2))
        {
            ok++;
            strcpy(cache[cn % 5].h, q);
            strcpy(cache[cn++ % 5].ip, r + 2);
            printf("[SERVER] %s -> %s\n", q, r + 2);
        }
        else
        {
            if (r[0] == 'N')
                nx++;
            else
                bad++;
            printf("%s\n", r);
        }
        printf("  Latency: %.3f ms (connect %.3f + query/response %.3f)\n", lat, ms(t0, t1), ms(t1, t2));
    }
    clock_gettime(CLOCK_MONOTONIC, &g1);

    double avg = net ? sum / net : 0, pay = sb + rb, ovh = net * 9 * 40; // ~9 packets x 40B headers
    printf("\n===== CLIENT METRICS =====\n");
    printf("Total queries        : %d\n", tot);
    printf("Cache hits / misses  : %d / %d\n", hit, tot - hit);
    printf("Cache hit ratio      : %.1f %%\n", tot ? hit * 100.0 / tot : 0);
    printf("Resolved / NXDOMAIN  : %d / %d\n", ok, nx);
    printf("Invalid / Timeout    : %d / %d\n", bad, tmo);
    printf("Connection failures  : %d\n", fail);
    printf("Timeout rate         : %.1f %%\n", (net + tmo) ? tmo * 100.0 / (net + tmo) : 0);
    printf("Latency min/avg/max  : %.3f / %.3f / %.3f ms\n", net ? mn : 0, avg, mx);
    printf("Avg connect time     : %.3f ms\n", net ? con / net : 0);
    printf("Avg query/response   : %.3f ms\n", net ? rsp / net : 0);
    printf("Handshake share      : %.1f %% of latency\n", sum ? con * 100 / sum : 0);
    printf("Time saved by cache  : ~%.3f ms\n", hit * avg);
    printf("Payload sent / recv  : %d / %d bytes\n", sb, rb);
    printf("Est. header overhead : %.0f bytes\n", ovh);
    printf("Protocol efficiency  : %.1f %%\n", pay + ovh ? pay * 100 / (pay + ovh) : 0);
    printf("Session time         : %.2f s\n", ms(g0, g1) / 1000);
}
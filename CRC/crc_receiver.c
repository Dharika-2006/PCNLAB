#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

#define G "10011"

void mod2(char *t, int len)
{
    for (int i = 0; i <= len - 5; i++)
        if (t[i] == '1')
            for (int j = 0; j < 5; j++)
                t[i + j] = (t[i + j] == G[j]) ? '0' : '1';
}

int main()
{
    int s = socket(AF_INET, SOCK_STREAM, 0), c, n, i, j, ok, o = 1, tot = 0, bad = 0;
    struct sockaddr_in a = {0};
    struct timespec t1, t2;
    double vt = 0;
    char f[450], t[450];

    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &o, sizeof(o));
    a.sin_family = AF_INET;
    a.sin_port = htons(8080);
    a.sin_addr.s_addr = INADDR_ANY;
    bind(s, (struct sockaddr *)&a, sizeof(a));
    listen(s, 5);
    printf("CRC receiver on 8080...\n");

    c = accept(s, NULL, NULL);
    while ((n = recv(c, f, sizeof(f) - 1, 0)) > 0)
    {
        f[n] = 0;
        strcpy(t, f);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        mod2(t, n);
        clock_gettime(CLOCK_MONOTONIC, &t2);
        vt += (t2.tv_sec - t1.tv_sec) * 1e6 + (t2.tv_nsec - t1.tv_nsec) / 1e3;
        for (ok = 1, i = n - 4; i < n; i++)
            if (t[i] != '0')
                ok = 0;
        tot++;
        if (!ok)
            bad++;
        printf("Received : %s\nRemainder: %s\n", f, t + n - 4);

        if (ok)
        {
            printf("Result   : NO ERROR\nData     : ");
            for (i = 0; i < n - 4; i += 8)
            {
                int ch = 0;
                for (j = 0; j < 8; j++)
                    ch = ch * 2 + (f[i + j] - '0');
                putchar(ch);
            }
            printf("\n");
        }
        else
            printf("Result   : ERROR DETECTED\n");
        send(c, ok ? "ACK" : "NAK", 3, 0);
    }
    printf("\n===== RECEIVER METRICS =====\n");
    printf("Frames received      : %d\n", tot);
    printf("Frames with errors   : %d (%.1f %%)\n", bad, tot ? bad * 100.0 / tot : 0);
    printf("Frames accepted      : %d\n", tot - bad);
    printf("Avg verify time      : %.3f us\n", tot ? vt / tot : 0);
    close(c);
}
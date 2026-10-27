#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

#define G "10011"
double ms(struct timespec a, struct timespec b) { return (b.tv_sec - a.tv_sec) * 1000.0 + (b.tv_nsec - a.tv_nsec) / 1e6; }

void mod2(char *t, int len)
{
    for (int i = 0; i <= len - 5; i++)
        if (t[i] == '1')
            for (int j = 0; j < 5; j++)
                t[i + j] = (t[i + j] == G[j]) ? '0' : '1';
}

int main()
{
    int s = socket(AF_INET, SOCK_STREAM, 0), i, j, n, pos, len, k = 0;
    int att = 0, ok = 0, sent_bits = 0, naks = 0, tmo = 0;
    struct sockaddr_in a = {0};
    struct timeval tv = {3, 0};
    struct timespec c1, c2, t0, t1, t2, te;
    char data[50], bin[450], t[450], tx[450], r[8];

    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    a.sin_family = AF_INET;
    a.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);
    connect(s, (struct sockaddr *)&a, sizeof(a));

    printf("Data: ");
    scanf("%49s", data);
    for (i = 0; data[i]; i++)
        for (j = 7; j >= 0; j--)
            bin[k++] = ((data[i] >> j) & 1) + '0';
    bin[k] = 0;

    strcpy(t, bin);
    strcat(t, "0000");
    clock_gettime(CLOCK_MONOTONIC, &c1);
    mod2(t, k + 4); // CRC computation timed
    clock_gettime(CLOCK_MONOTONIC, &c2);
    strcpy(bin + k, t + k);
    len = k + 4;
    printf("Binary : %.*s\nCRC    : %s\nFrame  : %s\n", k, bin, bin + k, bin);

    printf("Flip bit position (1-%d, 0 = none, -1 = lose frame): ", len);
    scanf("%d", &pos);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    while (att < 3 && !ok)
    {
        att++;
        strcpy(tx, bin);
        if (att == 1 && pos > 0 && pos <= len)
        {
            tx[pos - 1] ^= 1;
            printf("Bit %d flipped\n", pos);
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        if (att == 1 && pos < 0)
            printf("Frame lost (simulated)\n");
        else
        {
            send(s, tx, len, 0);
            sent_bits += len;
        }

        n = recv(s, r, sizeof(r) - 1, 0);
        clock_gettime(CLOCK_MONOTONIC, &t2);
        if (n > 0)
        {
            r[n] = 0;
            printf("Attempt %d: %s, RTT = %.3f ms\n", att, r, ms(t1, t2));
            if (!strcmp(r, "ACK"))
                ok = 1;
            else
                naks++;
        }
        else
        {
            printf("Attempt %d: timeout after %.0f ms\n", att, ms(t1, t2));
            tmo++;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &te);
    double tot = ms(t0, te) / 1000.0; // seconds

    printf("\n===== PERFORMANCE METRICS =====\n");
    printf("Data bits            : %d\n", k);
    printf("CRC bits             : 4\n");
    printf("Frame size           : %d bits\n", len);
    printf("CRC overhead         : %.2f %%\n", 400.0 / len);
    printf("CRC compute time     : %.3f us\n", ms(c1, c2) * 1000);
    printf("Attempts             : %d (retransmissions = %d)\n", att, att - 1);
    printf("NAKs / timeouts      : %d / %d\n", naks, tmo);
    printf("Total bits sent      : %d\n", sent_bits);
    printf("Efficiency           : %.2f %%\n", sent_bits ? k * 100.0 / sent_bits : 0);
    printf("Total time           : %.3f s\n", tot);
    printf("Throughput           : %.1f bps\n", sent_bits / tot);
    printf("Goodput              : %.1f bps\n", ok ? k / tot : 0);
    printf("Delivery             : %s\n", ok ? "SUCCESS" : "FAILED");
    close(s);
}
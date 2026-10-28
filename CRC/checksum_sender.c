#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

void pb(int v) { for (int j = 7; j >= 0; j--) putchar((v >> j & 1) + '0'); }

int main() {
    int s = socket(AF_INET, SOCK_STREAM, 0), i, j, n, k, pos, len, att, w, raw, cy, sum = 0, ok = 0;
    struct sockaddr_in a = {0};
    struct timeval tv = {3, 0};                    // 3 sec ACK timer
    char d[50], f[450], tx[450], r[8];

    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    a.sin_family = AF_INET; a.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);
    connect(s, (struct sockaddr *)&a, sizeof(a));

    printf("Data: "); scanf("%49s", d); k = strlen(d);
    for (i = 0; i < k; i++) {                      // 8-bit words, 1's complement add
        w = (unsigned char)d[i];
        raw = sum + w; cy = raw >> 8; sum = (raw & 0xFF) + cy;     // end-around carry
        printf("Word %d: ", i + 1); pb(w);
        printf("  raw=%3d  carry=%d  sum=", raw, cy); pb(sum); printf("\n");
    }
    printf("Final sum : "); pb(sum);
    printf("\nChecksum  : "); pb(~sum & 0xFF); printf("   (1's complement of sum)\n");

    for (i = 0, n = 0; i <= k; i++) {              // frame = data words + checksum word
        w = i < k ? (unsigned char)d[i] : (~sum & 0xFF);
        for (j = 7; j >= 0; j--) f[n++] = (w >> j & 1) + '0';
    }
    f[n] = 0; len = n;
    printf("Frame     : %s\n", f);
    printf("Flip bit position (1-%d, 0 = none, -1 = lose frame): ", len);
    scanf("%d", &pos);

    for (att = 1; att <= 3 && !ok; att++) {
        strcpy(tx, f);
        if (att == 1 && pos > 0 && pos <= len) { tx[pos - 1] ^= 1; printf("Bit %d flipped\n", pos); }
        if (att == 1 && pos < 0) printf("Frame lost (simulated)\n");
        else { send(s, tx, len, 0); printf("Attempt %d: frame sent\n", att); }
        n = recv(s, r, sizeof(r) - 1, 0);
        if (n > 0) { r[n] = 0; printf("Receiver: %s\n", r); ok = !strcmp(r, "ACK"); }
        else printf("Timeout, retransmitting\n");
        if (!ok && att < 3) printf("Retransmitting\n");
    }
    printf(ok ? "Delivered after %d attempt(s)\n" : "Failed after 3 attempts\n", att - 1);
    close(s);
}

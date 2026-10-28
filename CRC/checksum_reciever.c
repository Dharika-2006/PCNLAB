#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

void pb(int v) { for (int j = 7; j >= 0; j--) putchar((v >> j & 1) + '0'); }

int main() {
    int s = socket(AF_INET, SOCK_STREAM, 0), c, n, i, j, w, raw, cy, sum, ok, o = 1;
    struct sockaddr_in a = {0};
    char f[450];

    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &o, sizeof(o));
    a.sin_family = AF_INET; a.sin_port = htons(8080); a.sin_addr.s_addr = INADDR_ANY;
    bind(s, (struct sockaddr *)&a, sizeof(a)); listen(s, 5);
    printf("Checksum receiver on 8080...\n");

    c = accept(s, NULL, NULL);
    while ((n = recv(c, f, sizeof(f) - 1, 0)) > 0) {
        f[n] = 0; sum = 0;
        printf("\nReceived: %s\n", f);
        for (i = 0; i + 8 <= n; i += 8) {          // add every word, checksum included
            for (w = 0, j = 0; j < 8; j++) w = w * 2 + (f[i + j] - '0');
            raw = sum + w; cy = raw >> 8; sum = (raw & 0xFF) + cy;
            printf("Word %d: ", i / 8 + 1); pb(w);
            printf("  raw=%3d  carry=%d  sum=", raw, cy); pb(sum); printf("\n");
        }
        printf("Final sum  : "); pb(sum);
        printf("\nComplement : "); pb(~sum & 0xFF);
        ok = (sum == 0xFF);                        // all 1s -> complement is 0 -> no error
        printf("\nResult     : %s\n", ok ? "NO ERROR" : "ERROR DETECTED");
        if (ok) {
            printf("Data       : ");
            for (i = 0; i + 8 < n; i += 8) {       // last word is the checksum
                for (w = 0, j = 0; j < 8; j++) w = w * 2 + (f[i + j] - '0');
                putchar(w);
            }
            printf("\n");
        }
        send(c, ok ? "ACK" : "NAK", 3, 0);
    }
    close(c);
}

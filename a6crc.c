/* Usage: ./a6 recv   (terminal 1)   |   ./a6 send   (terminal 2) */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#define PORT 9090

void crc(char *bits, char *rem)
{ /* G(x) = x^4 + x + 1 = 10011 */
    char *g = "10011", t[1100];
    strcpy(t, bits);
    int n = strlen(t);
    for (int i = 0; i + 5 <= n; i++)
        if (t[i] == '1')
            for (int j = 0; j < 5; j++)
                t[i + j] = (t[i + j] == g[j]) ? '0' : '1';
    strcpy(rem, t + n - 4);
}

int main(int argc, char **argv)
{
    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = {AF_INET, htons(PORT)};

    if (argv[1][0] == 'r')
    {
        a.sin_addr.s_addr = INADDR_ANY;
        bind(s, (void *)&a, sizeof a);
        listen(s, 1);
        int c = accept(s, NULL, NULL), n, len = 0;
        char buf[1100] = {0}, rem[8];
        while ((n = recv(c, buf + len, 1000 - len, 0)) > 0)
            len += n;
        printf("Received: %s\n", buf);
        crc(buf, rem);
        printf("Remainder = %s -> %s\n", rem, strcmp(rem, "0000") ? "Error detected" : "Error-free");
    }
    else
    {
        a.sin_addr.s_addr = inet_addr("127.0.0.1");
        char msg[100], b[1100], t[1100], rem[8];
        int pos, n = 0;
        printf("Data: ");
        scanf(" %99[^\n]", msg);
        for (char *p = msg; *p; p++)
            for (int k = 7; k >= 0; k--)
                b[n++] = ((*p >> k) & 1) + '0';
        b[n] = 0;
        printf("Binary: %s\n", b);
        sprintf(t, "%s0000", b);
        crc(t, rem);
        printf("CRC = %s\n", rem);
        strcat(b, rem);
        printf("Bit position to flip (0 = none): ");
        scanf("%d", &pos);
        if (pos > 0)
            b[pos - 1] ^= 1; /* '0'^1='1', '1'^1='0' */
        connect(s, (void *)&a, sizeof a);
        send(s, b, strlen(b), 0);
    }
    close(s);
}
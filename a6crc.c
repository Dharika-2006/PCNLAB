#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9090

void crc(char *b, char *r)
{
    char g[] = "10011", t[1100];
    strcpy(t, b);
    int n = strlen(t);

    for (int i = 0; i < n - 4; i++)
        if (t[i] == '1')
            for (int j = 0; j < 5; j++)
                t[i + j] = (t[i + j] == g[j]) ? '0' : '1';

    strcpy(r, t + n - 4);
}

int main(int argc, char *argv[])
{
    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port = htons(PORT);

    if (argc < 2)
        return 1;

    if (strcmp(argv[1], "recv") == 0)
    {
        a.sin_addr.s_addr = INADDR_ANY;
        bind(s, (void *)&a, sizeof(a));
        listen(s, 1);

        int c = accept(s, NULL, NULL), n, len = 0;
        char b[1100], r[5];

        while ((n = recv(c, b + len, sizeof(b) - 1 - len, 0)) > 0)
            len += n;

        b[len] = '\0';
        crc(b, r);
        printf("Received bits: %s\nCRC remainder: %s\n", b, r);
        printf("%s\n", strcmp(r, "0000") ? "Error detected" : "Error-free");
        close(c);
    }
    else
    {
        a.sin_addr.s_addr = inet_addr("127.0.0.1");

        char msg[100], b[1100], t[1100], r[5];
        printf("Enter data: ");
        fgets(msg, sizeof(msg), stdin);
        msg[strcspn(msg, "\n")] = '\0';

        int n = 0;
        for (int i = 0; msg[i]; i++)
            for (int k = 7; k >= 0; k--)
                b[n++] = ((msg[i] >> k) & 1) + '0';
        b[n] = '\0';

        strcpy(t, b);
        strcat(t, "0000");
        crc(t, r);
        strcat(b, r);

        connect(s, (void *)&a, sizeof(a));
        send(s, b, strlen(b), 0);
    }

    close(s);
    return 0;
}

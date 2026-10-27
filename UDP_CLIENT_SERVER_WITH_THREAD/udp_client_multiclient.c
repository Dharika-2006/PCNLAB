#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
volatile int acked;
int s;
void *rx(void *p)
{ // thread: ACK or solution
    char r[600];
    int n;
    while ((n = recv(s, r, sizeof(r) - 1, 0)) > 0)
    {
        r[n] = 0;
        if (!strcmp(r, "ACK"))
            acked = 1;
        else
            printf("%s\n", r);
    }
    return NULL;
}
int main()
{
    struct sockaddr_in a = {0};
    pthread_t th;
    char u[50], m[100], b[200];
    int i, k;
    s = socket(AF_INET, SOCK_DGRAM, 0);
    a.sin_family = AF_INET;
    a.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);
    connect(s, (struct sockaddr *)&a, sizeof(a));
    printf("Username: ");
    scanf("%49s", u);
    getchar();
    pthread_create(&th, NULL, rx, NULL);
    printf("Enter problem:\n");
    while (fgets(m, sizeof(m), stdin))
    {
        m[strcspn(m, "\n")] = 0;
        snprintf(b, sizeof(b), "%s %s", u, m);
        for (i = 0, acked = 0; i < 3 && !acked; i++)
        { // up to 3 tries
            send(s, b, strlen(b), 0);
            for (k = 0; k < 20 && !acked; k++)
                usleep(100000); // 2 sec ACK timer
            if (!acked)
                printf("Timeout, retry %d/3\n", i + 1);
        }
        if (!acked)
            printf("No ACK\n");
    }
}
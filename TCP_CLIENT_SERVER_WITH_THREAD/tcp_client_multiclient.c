#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
void *rx(void *p)
{
    char r[600];
    int n, s = (int)(long)p;
    while ((n = recv(s, r, sizeof(r) - 1, 0)) > 0)
    {
        r[n] = 0;
        printf("%s", r);
    }
    printf("Disconnected\n");
    _exit(0);
}
int main()
{
    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = {0};
    pthread_t th;
    char u[50], m[100], b[200];
    a.sin_family = AF_INET;
    a.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);
    if (connect(s, (struct sockaddr *)&a, sizeof(a)) < 0)
    {
        printf("Connect failed\n");
        return 1;
    }
    printf("Username: ");
    scanf("%49s", u);
    getchar();
    pthread_create(&th, NULL, rx, (void *)(long)s);
    printf("Enter problem:\n");
    while (fgets(m, sizeof(m), stdin))
    {
        m[strcspn(m, "\n")] = 0;
        snprintf(b, sizeof(b), "%s %s", u, m);
        send(s, b, strlen(b), 0);
    }
    close(s);
}
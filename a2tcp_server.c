#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/time.h>
#define PORT 8080
#define DROP 1 /* 1 = drop first reply to demo retransmission, 0 = normal */

void *handle(void *arg)
{
    int c = *(int *)arg;
    free(arg);
    struct timeval tv = {3, 0}; /* server timer: 3 s */
    setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    char roll[64] = {0}, line[256], out[256] = "Student Record Not Found", ack[16];
    int n = read(c, roll, sizeof roll - 1);
    if (n > 0)
    {
        roll[strcspn(roll, "\r\n")] = 0;
        FILE *f = fopen("students.csv", "r");
        while (f && fgets(line, sizeof line, f))
        {
            char tmp[256], *r;
            strcpy(tmp, line);
            r = strtok(tmp, ",");
            if (r && !strcmp(r, roll))
            {
                strcpy(out, line);
                break;
            }
        }
        if (f)
            fclose(f);
        for (int t = 0; t < 3; t++)
        { /* send reply, wait for ACK, retransmit */
            if (DROP && t == 0)
                puts("Reply dropped (simulated)");
            else
                write(c, out, strlen(out));
            n = read(c, ack, sizeof ack - 1);
            if (n > 0 && ack[0] == 'A')
            {
                puts("ACK received");
                break;
            }
            puts("No ACK, retransmitting reply");
        }
    }
    close(c);
    return NULL;
}

int main()
{
    int s = socket(AF_INET, SOCK_STREAM, 0), one = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    struct sockaddr_in a = {AF_INET, htons(PORT), {INADDR_ANY}};
    bind(s, (void *)&a, sizeof a);
    listen(s, 5);
    while (1)
    {
        int *c = malloc(sizeof(int));
        *c = accept(s, NULL, NULL);
        pthread_t t;
        pthread_create(&t, NULL, handle, c);
        pthread_detach(t);
    }
}

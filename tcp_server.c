
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080

void *handle(void *arg)
{
    int c = *(int *)arg;
    free(arg);

    char roll[64] = {0};
    char line[256];
    char out[256] = "Student Record Not Found";

    int n = read(c, roll, sizeof roll - 1);

    if (n > 0)
    {
        roll[strcspn(roll, "\r\n")] = '\0';

        FILE *f = fopen("students.csv", "r");

        while (f && fgets(line, sizeof line, f))
        {
            char tmp[256];
            char *r;

            strcpy(tmp, line);
            r = strtok(tmp, ",");

            if (r && strcmp(r, roll) == 0)
            {
                strcpy(out, line);
                break;
            }
        }
        if (f)
            fclose(f);

        write(c, out, strlen(out));
    }

    close(c);

    return NULL;
}

int main()
{
    int s = socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;

    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);

    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port = htons(PORT);
    a.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(s, (struct sockaddr *)&a, sizeof a);
    listen(s, 5);

    while (1)
    {
        int *c = malloc(sizeof(int));

        *c = accept(s, NULL, NULL);

        pthread_t t;
        pthread_create(&t, NULL, handle, c);
        pthread_detach(t);
    }

    close(s);
    return 0;
}

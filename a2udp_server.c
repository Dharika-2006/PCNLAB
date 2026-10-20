#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define PORT 8081

int main()
{
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in a = {AF_INET, htons(PORT), {INADDR_ANY}}, c;
    socklen_t l = sizeof c;
    bind(s, (void *)&a, sizeof a);
    char roll[64], line[256], out[256];
    while (1)
    {
        int n = recvfrom(s, roll, sizeof roll - 1, 0, (void *)&c, &l);
        if (n < 0)
            continue;
        roll[n] = 0;
        roll[strcspn(roll, "\r\n")] = 0;
        strcpy(out, "Student Record Not Found");
        FILE *f = fopen("students.csv", "r");
        while (f && fgets(line, sizeof line, f))
        {
            char *r = strtok(line, ","), *name = strtok(NULL, ",");
            if (r && name && !strcmp(r, roll))
            {
                sprintf(out, "%s,%s", r, name);
                break;
            }
        }
        if (f)
            fclose(f);
        sendto(s, out, strlen(out), 0, (void *)&c, l);
    }
}
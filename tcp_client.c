#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main()
{
    int s = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server
    if (connect(s, (struct sockaddr *)&server, sizeof server) < 0)
    {
        perror("Connection failed");
        close(s);
        return 1;
    }

    char roll[64];
    char response[256] = {0};

    printf("Enter roll number: ");
    fgets(roll, sizeof roll, stdin);

    // Send roll number to server
    write(s, roll, strlen(roll));

    // Receive student record
    int n = read(s, response, sizeof response - 1);

    if (n > 0)
    {
        response[n] = '\0';
        printf("Student Record: %s\n", response);
    }
    else
    {
        printf("No response from server.\n");
    }

    close(s);

    return 0;
}
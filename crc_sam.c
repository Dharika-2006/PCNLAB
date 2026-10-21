#include <stdio.h>
#include <string.h>

#define MAX 256

// Perform modulo-2 division and return the remainder
void crcRemainder(const char *bits, const char *generator, char *remainder)
{
    int n = strlen(bits);
    int g = strlen(generator);
    char temp[MAX];

    strcpy(temp, bits);

    for (int i = 0; i <= n - g; i++) {
        if (temp[i] == '1') {
            for (int j = 0; j < g; j++) {
                temp[i + j] =
                    (temp[i + j] == generator[j]) ? '0' : '1';
            }
        }
    }

    // Remainder is the last g-1 bits
    strncpy(remainder, temp + n - (g - 1), g - 1);
    remainder[g - 1] = '\0';
}

int validBinary(const char *s)
{
    if (strlen(s) == 0)
        return 0;

    for (int i = 0; s[i]; i++) {
        if (s[i] != '0' && s[i] != '1')
            return 0;
    }

    return 1;
}

int allZero(const char *s)
{
    for (int i = 0; s[i]; i++) {
        if (s[i] != '0')
            return 0;
    }

    return 1;
}

int main(void)
{
    char data[MAX], generator[MAX];
    char padded[MAX], crc[MAX], codeword[MAX];
    char received[MAX], check[MAX];

    printf("Enter data bits: ");
    scanf("%255s", data);

    printf("Enter generator polynomial bits: ");
    scanf("%255s", generator);

    if (!validBinary(data) || !validBinary(generator) ||
        strlen(generator) < 2 || generator[0] != '1') {
        printf("Invalid input. Enter binary strings; generator must start with 1.\n");
        return 1;
    }

    int dataLen = strlen(data);
    int genLen = strlen(generator);

    if (dataLen + genLen >= MAX) {
        printf("Input is too long.\n");
        return 1;
    }

    // Append genLen-1 zeros to the data
    strcpy(padded, data);

    for (int i = 0; i < genLen - 1; i++)
        padded[dataLen + i] = '0';

    padded[dataLen + genLen - 1] = '\0';

    // Calculate CRC bits
    crcRemainder(padded, generator, crc);

    // Form transmitted codeword = data + CRC
    strcpy(codeword, data);
    strcat(codeword, crc);

    printf("\n--- CRC Sender ---\n");
    printf("Original data : %s\n", data);
    printf("Generator     : %s\n", generator);
    printf("CRC bits      : %s\n", crc);
    printf("Codeword      : %s\n", codeword);

    // Get receiver input, so the user can enter a possibly corrupted word
    printf("\nEnter received codeword (same length, or copy the transmitted codeword): ");
    scanf("%255s", received);

    if (!validBinary(received) || strlen(received) != strlen(codeword)) {
        printf("Invalid received codeword length or bits.\n");
        return 1;
    }

    crcRemainder(received, generator, check);

    printf("\n--- Receiver Check ---\n");
    printf("Received word : %s\n", received);
    printf("Remainder     : %s\n", check);

    if (allZero(check))
        printf("Result        : No error detected\n");
    else
        printf("Result        : Error detected\n");

    // Performance matrix: test no error and every possible single-bit flip
    printf("\n--- CRC Performance Matrix ---\n");
    printf("Test\tBit flipped\tReceived codeword\tRemainder\tResult\n");

    int totalTests = 0;
    int detected = 0;

    // Test 0: uncorrupted codeword
    strcpy(received, codeword);
    crcRemainder(received, generator, check);

    printf("%d\t%s\t\t%s\t%s\t%s\n",
           totalTests, "None", received, check,
           allZero(check) ? "Pass" : "Fail");

    totalTests++;

    // Test each bit position by flipping one bit
    for (int pos = 0; pos < (int)strlen(codeword); pos++) {
        strcpy(received, codeword);

        // Flip bit: 0 becomes 1, 1 becomes 0
        received[pos] = (received[pos] == '0') ? '1' : '0';

        crcRemainder(received, generator, check);

        int errorDetected = !allZero(check);
        if (errorDetected)
            detected++;

        printf("%d\t%d\t\t%s\t%s\t%s\n",
               totalTests, pos + 1, received, check,
               errorDetected ? "Error detected" : "Not detected");

        totalTests++;
    }

    printf("\n--- Summary ---\n");
    printf("Single-bit error tests : %d\n", totalTests - 1);
    printf("Errors detected        : %d\n", detected);
    printf("Detection percentage  : %.2f%%\n",
           (totalTests > 1)
               ? 100.0 * detected / (totalTests - 1)
               : 0.0);

    return 0;
}

// C program to read a file line by line

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    FILE *fp = fopen("employee-codes.txt", "r");
    if (fp == NULL) {
        perror("Failed to open file");
        return 1;
    }

    int num;
    while (fscanf(fp, "%d", &num) != EOF) {
        printf("%d\n", num);
    }

    fclose(fp);
    return 0;

 
}
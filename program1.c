#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <int1> <int2> <int3>\n", argv[0]);
        return 1;
    }

    int num1 = atoi(argv[1]);
    int num2 = atoi(argv[2]);
    int num3 = atoi(argv[3]);

    int sum = num1 + num2 + num3;

    printf("%d", sum);

    return 0;
}


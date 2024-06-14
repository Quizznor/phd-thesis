#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{

    int pmt_mask = atoi(argv[1]);
    printf("number received: %i\n", pmt_mask);

    for (int i=0; i<3; i++)
    {
        printf("PMT #%i: mask = %i\n", i + 1, (pmt_mask >> i) & 0x1);
    }

    return 0;
}
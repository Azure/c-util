// Copyright(C) Microsoft Corporation.All rights reserved.

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    int32_t result = 0;

    if (argc > 1)
    {
        for (int i = 1; i < argc; ++i)
        {
            if (i == argc - 1 &&
                sscanf(argv[i], "%" SCNd32 "", &result) == 1)
            {
                // Last argument was numeric, treat that as the return value
            }
            else
            {
                // Simple program just echos the arguments back
                printf("%s\n", argv[i]);
            }
        }
    }

    return (int)result;
}

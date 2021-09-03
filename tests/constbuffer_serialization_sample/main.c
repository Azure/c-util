// Copyright(C) Microsoft Corporation.All rights reserved.

#include <inttypes.h>
#include <stdlib.h>

#include "c_logging/xlogging.h"

#include "c_util/constbuffer.h"

/*below sample contains a working code of how to serialize consecutive CONSTBUFFER_HANDLEs in an ever-growing array using CONSTBUFFER_to_buffer*/

/*forward*/
static void* awesome_alloc(size_t size, void* context);
typedef struct AWESOME_ALLOC_CONTEXT_TAG
{
    unsigned char* buffer;
    size_t capacity; /*total number of bytes of buffer, initially 0*/
    size_t size; /*used bytes out of capacity*/

    /*statistics*/
    uint32_t n_reallocs;
} AWESOME_ALLOC_CONTEXT;


int main(void)
{
    AWESOME_ALLOC_CONTEXT awesome_alloc_context = { 0 };

    unsigned char source[1000];
    for (uint32_t i = 0; i < 1000 ;i++)
    {
        /*construct random size constbuffer from source*/
        uint32_t size = 1 + (rand() % sizeof(source));

        CONSTBUFFER_HANDLE h = CONSTBUFFER_Create(source, size);

        if (h == NULL)
        {
            LogError("failure creating CONSTBUFFER_HANDLE h");
        }
        else
        {
            /*serialize it into the context*/
            uint32_t serialized_size;
            if (CONSTBUFFER_to_buffer(h, awesome_alloc, &awesome_alloc_context, &serialized_size) == NULL)
            {
                LogError("failure in serialization");
            }
            else
            {
                /*all ok, has been serialized*/
            }

            CONSTBUFFER_DecRef(h);
        }
    }

    (void)printf("to add 1000 array there were needed %" PRIu32 " reallocs\n", awesome_alloc_context.n_reallocs);

    return 0;
}

#define GROW_BY 1000

#define LOCALMAX(x, y) ((x)<(y)?(y):(x))

static void* awesome_alloc(size_t size, void* context)
{
    /*here context contains a buffer that *might* grow*/
    AWESOME_ALLOC_CONTEXT* awesome_alloc_context = context;
    (void)printf("requesting %zu size where buffer=%p, capacity=%zu, size=%zu\n",
        size, awesome_alloc_context->buffer, awesome_alloc_context->capacity, awesome_alloc_context->size);
    if (awesome_alloc_context->size + size <= awesome_alloc_context->capacity)
    {
        /*just return previously allocated memory, no new realloc needed*/

        awesome_alloc_context->size += size; /*increase usage*/
        return awesome_alloc_context->buffer + awesome_alloc_context->size - size;
    }
    else
    {
        (void)printf("realloc!\n");
        unsigned char* temp = realloc(awesome_alloc_context->buffer, awesome_alloc_context->capacity + LOCALMAX(GROW_BY, size));
        if (temp == NULL)
        {
            LogError("cannot realloc");
            return NULL;
        }
        else
        {
            awesome_alloc_context->n_reallocs++;
            awesome_alloc_context->buffer = temp;
            awesome_alloc_context->capacity += LOCALMAX(GROW_BY, size);
            awesome_alloc_context->size += size;
            return awesome_alloc_context->buffer + awesome_alloc_context->size - size;
        }
    }
}

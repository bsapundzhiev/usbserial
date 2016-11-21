/*ring buffer example*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rbuff.h"

void rbuf_init(rbuf_t *b)
{
    memset(&b->buf, 0, BUFSIZE);
    b->pIn = b->pOut = b->buf;
    b->pEnd = &b->buf[BUFSIZE];
    b->len = 0;
}

int rbuf_is_full(rbuf_t *b)
{
    return b->len == BUFSIZE;
}

int rbuf_is_empty(rbuf_t *b)
{
	return b->len == 0;
}

int rbuf_put(rbuf_t *b, char c)
{
    if (b->pIn == b->pOut && rbuf_is_full(b)) {
        return 0;
    }

    *b->pIn++ = c;
    b->len++;
    if (b->pIn >= b->pEnd) {
        b->pIn = b->buf;
    }
    return 1;
}

int rbuf_get(rbuf_t *b, char *pc)
{
    if (b->pIn == b->pOut && rbuf_is_empty(b)) {
        return 0;
    }

    *pc = *b->pOut++;
    b->len--;
    if (b->pOut >= b->pEnd) {
        b->pOut = b->buf;
    }
    return 1;
}

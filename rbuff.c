/*ring buffer example*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rbuff.h"

void buf_init(t_rbuf *b)
{
    memset(&b->buf, 0, BUFSIZE);
    b->pIn = b->pOut = b->buf;
    b->pEnd = &b->buf[BUFSIZE];
    b->len = 0;
}

int buf_is_full(t_rbuf *b)
{
    return b->len == BUFSIZE;
}

int buf_put(t_rbuf *b, char c)
{
    if (b->pIn == b->pOut && buf_is_full(b)) {
        //printf("put full\n");
        return 0;
    }

    *b->pIn++ = c;
    b->len++;
    if (b->pIn >= b->pEnd) {
        b->pIn = b->buf;
    }
    return 1;
}

int buf_get(t_rbuf *b, char *pc)
{
    if (b->len == 0/*b->pIn == b->pOut && !buf_is_full(b)*/) {
       //printf("get full\n");
        return 0;
    }

    *pc = *b->pOut++;
    b->len--;
    if (b->pOut >= b->pEnd) {
        b->pOut = b->buf;
    }
    return 1;
}
/*
int main() {
    char ch, i, j;
    t_rbuf buf;

    buf_init(&buf);
    buf_put(&buf, 'a');
    buf_put(&buf, 'b');
    buf_put(&buf, 'c');
    buf_put(&buf, 'd');
    buf_put(&buf, 'e');
    j = buf.len;
    for(i =0; i < j; i++) {
        buf_get(&buf, &ch);
        printf("%c\n", ch);
    }
    system("pause");
    return 0;
}
*/
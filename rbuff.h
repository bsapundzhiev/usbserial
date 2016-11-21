#ifndef _RBUFF_H
#define _RBUFF_H

#define BUFSIZE 10

typedef struct _rbuf {
    char buf[BUFSIZE];
    char *pIn;
    char *pOut;
    char *pEnd;
    int len;
} rbuf_t;

void rbuf_init(rbuf_t *b);
int rbuf_put(rbuf_t *b, char c);
int rbuf_get(rbuf_t *b, char *pc);
int rbuf_is_full(rbuf_t *b);
int rbuf_is_empty(rbuf_t *b);
#endif

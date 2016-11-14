#ifndef _RBUFF_H
#define _RBUFF_H

#define BUFSIZE 256

typedef struct _rbuf {
    char buf[BUFSIZE];
    char *pIn;
    char *pOut;
    char *pEnd;
    int len;
} t_rbuf;

void buf_init(t_rbuf *b);
int buf_put(t_rbuf *b, char c);
int buf_get(t_rbuf *b, char *pc);
int buf_is_full(t_rbuf *b);

#endif

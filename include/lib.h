#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define err_quit(m) { perror(m); exit(-1); }

#define MAXLINE 1024
#define SEND_TIME 32
#define CHECK_ACK_TIME 256
#define MAX(a, b) (a>b?a:b)

typedef struct {
    int     file_no;
    int     seq_no;
    int     eof;
    char    data[MAXLINE];
} pkt_t;

typedef struct {
    int file_no;
    int seq_no;
} ack_t;

void printpkt(pkt_t* p){
    printf("FILE: %d, SEQ: %d, EOF:%d\n", p->file_no, p->seq_no, p->eof);
}
void printack(ack_t* a){
    printf("ACK ===== FILE: %d, SEQ: %d\n", a->file_no, a->seq_no);
}
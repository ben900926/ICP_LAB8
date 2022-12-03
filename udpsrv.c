/*
 * Lab problem set for INP course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */
#include "include/lib.h"

static int file_no = 0, seq_no = 0;
static int s = -1;
static struct sockaddr_in sin;

void send_ack(int sig) {
	ack_t ack = {.file_no = file_no, .seq_no = seq_no+1};
	sendto(s, (void*) &ack, sizeof(ack), 0, (struct sockaddr*) &sin, sizeof(sin));
	printack(&ack);

	alarm(1);
}

void do_nothing(int sig) {}

int main(int argc, char *argv[]) {

	if(argc < 4) {
		return -fprintf(stderr, "usage: %s <path-to-store-files> <total-number-of-files> <port>\n", argv[0]);
	}

	char	*path = argv[1];
	char 	filepath[30];
	int		nfiles = atoi(argv[2]);
	int 	fw, file_no, seq_no;
	int 	rlen;

	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(strtol(argv[argc-1], NULL, 0));

	if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		err_quit("socket");

	if(bind(s, (struct sockaddr*) &sin, sizeof(sin)) < 0)
		err_quit("bind");

	signal(SIGALRM, send_ack);

	alarm(1);
	
	/* handle receive file */
	for (file_no = 0, seq_no = 0; file_no < nfiles; ) {
		
		sprintf(filepath, "%s/%06d", path, file_no);
		
		fw = creat(filepath, S_IRUSR | S_IWUSR);

		if (fw == -1) {
			err_quit("Can't creat file");
			continue;
		}

		/* deal with packet */
		struct sockaddr_in csin;
		socklen_t csinlen = sizeof(csin);
		pkt_t pkt;
		for ( ; ; ) {

			if((rlen = recvfrom(s, (void*) &pkt, sizeof(pkt), 0, (struct sockaddr*) &csin, &csinlen)) <= 0) {
				perror("recvfrom");
				continue;
			}

			printf("Receive packet:\n");
			printpkt(&pkt);

			/* Ack and write file */
			if (file_no == pkt.file_no && seq_no == pkt.seq_no)
			{
				if (write(fw, pkt.data, strlen(pkt.data)) < 0)
					perror("write");
				
				seq_no++;

				if (pkt.eof == 1) {
					file_no++;
					break;	
				}
			}
			
			memset(&pkt, 0, sizeof(pkt));
		}
		
		close(fw);
	}

	signal(SIGALRM, do_nothing);

	for ( ; ; );

	close(s);
}
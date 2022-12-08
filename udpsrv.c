/*
 * Lab problem set for INP course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */
#include "include/lib.h"

// current received file and seq
static int file_no = 0, seq_no = 0;
static int s = -1;
static struct sockaddr_in sin;

static struct sockaddr_in csin;
static socklen_t csinlen = sizeof(csin);

void send_ack(int sig) {
	//printf("SENDING ACK --> FILE: %d, SEQ: %d\n", file_no, seq_no);
	ack_t ack = {.file_no = file_no, .seq_no = seq_no};
	sendto(s, (void*) &ack, sizeof(ack), 0, (struct sockaddr*) &csin, sizeof(csin));
	// printack(&ack);

}

int main(int argc, char *argv[]) {

	if(argc < 4) {
		return -fprintf(stderr, "usage: %s <path-to-store-files> <total-number-of-files> <port>\n", argv[0]);
	}

	char	*path = argv[1];
	char 	filepath[30];
	int		nfiles = atoi(argv[2]);
	int 	fw;
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

	// send 4 time to srv in 1024 usec
	ualarm( (useconds_t)( CHECK_ACK_TIME ), (useconds_t) CHECK_ACK_TIME );
	
	/* handle receive file */
	for (file_no = 0, seq_no = 0; file_no < nfiles; ) {
		
		sprintf(filepath, "%s/%06d", path, file_no);
		
		fw = creat(filepath, S_IRUSR | S_IWUSR);

		if (fw == -1) {
			err_quit("Can't creat file");
			continue;
		}

		/* Deal with packets */
		pkt_t pkt;
		for ( ; ; ) {

			/* Receive packet */
			if((rlen = recvfrom(s, (void*) &pkt, sizeof(pkt), 0, (struct sockaddr*) &csin, &csinlen)) <= 0) {
				perror("recvfrom");
				continue;
			}
			/* send ack right after server recv something
			if(pkt.eof==0){
				ack_t ack = {.file_no = file_no, .seq_no = seq_no};
				sendto(s, (void*) &ack, sizeof(ack), 0, (struct sockaddr*) &csin, sizeof(csin));
			}*/

			/* Store */
			// write to different file?
			if (file_no == pkt.file_no && seq_no == pkt.seq_no)
			{
				printf("Receive packet name: %d\tseq: %d\teof: %d\n", pkt.file_no, pkt.seq_no, pkt.eof);

				if (pkt.eof == 1) {
					file_no++;
					seq_no = 0;
					break;	
				}

				if (write(fw, pkt.data, strlen(pkt.data)) < 0)
					perror("write");
				
				seq_no++;

			}
			/* keep passing garbage
			else{
				printf("garbage: %d\tseq: %d\teof: %d\n", pkt.file_no, pkt.seq_no, pkt.eof);
			}*/
			
			memset(&pkt, 0, sizeof(pkt));
		}
		
		close(fw);
	}
	
	send_ack(SIGALRM);

	close(s);
}

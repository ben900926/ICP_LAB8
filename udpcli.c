/*
 * Lab problem set for INP course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */
#include "include/lib.h"

static int file_no = 0, seq_no = 0;
static int s = -1;
static struct sockaddr_in sin;
static int nfiles = INT_MAX;
static ack_t ack;

// every 256ms check for ack

void recv_ack(int sig) {
	struct sockaddr_in csin;
	socklen_t csinlen = sizeof(csin);
	int rlen;
	//printf("RECV ACK... sleep\n");
	//usleep(256);

	static int recordfn = 0, recordsn = 0;
	while ((rlen = recvfrom(s, (void*) &ack, sizeof(ack), MSG_DONTWAIT, (struct sockaddr*) &csin, &csinlen)) > 0) {
		if (recordfn < ack.file_no || (recordfn == ack.file_no && recordsn < ack.seq_no)) {
			recordfn = ack.file_no;
			recordsn = ack.seq_no;
			printf("Receive ACK ");
			printack(&ack);
		}

		if (file_no == nfiles) {
			exit(0);
		}

		memset(&ack, 0, sizeof(ack));
	}
	// update with largest ack received
	file_no = recordfn;
	seq_no = recordsn;
}


int main(int argc, char *argv[]) {
	if(argc < 5) {
		return -fprintf(stderr, "usage: %s <path-to-read-files> <total-number-of-files> <port> <server-ip-address>\n", argv[0]);
	}

	char 	*path = argv[1];
	char 	filepath[30];
	int 	fr;

	nfiles = atoi(argv[2]);

	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(strtol(argv[argc-2], NULL, 0));
	if(inet_pton(AF_INET, argv[argc-1], &sin.sin_addr) != 1) {
		return -fprintf(stderr, "** cannot convert IPv4 address for %s\n", argv[argc-1]);
	}

	if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		err_quit("socket");

	// check for ack every 1024 us
	signal(SIGALRM, recv_ack);
	ualarm( (useconds_t)( CHECK_ACK_TIME * 4 ), (useconds_t) CHECK_ACK_TIME * 4 );

	for (file_no = 0, seq_no = 0; file_no <= nfiles; ) {
		if (file_no == nfiles) continue;
		
		// open data
		sprintf(filepath, "%s/%06d", path, file_no);
		fr = open(filepath, O_RDONLY);
		if (fr == -1)
		{	
			err_quit("No such file");
		}
		// define pkt
		pkt_t pkt = {.file_no = file_no, .seq_no = seq_no, .eof = 0};
		// have sent to server
		int skip_seq = seq_no;
		//seq_no = 0;
		// each seq send for 32 times
		//printf("SEND FILE...");
		while (read(fr, pkt.data, sizeof(pkt.data)) > 0) {

			//if (file_no == ack.file_no && seq_no < ack.seq_no) continue;
			if(skip_seq>0){
				skip_seq--;
				continue;
			}

			for (int i = 0; i < SEND_TIME; i++) {
				if(sendto(s, (void*) &pkt, sizeof(pkt), 0, (struct sockaddr*) &sin, sizeof(sin)) < 0)
					perror("sendto");
				usleep(1);
			}
			
			pkt.seq_no++;
			seq_no++;

			memset(&pkt.data, 0, sizeof(pkt.data));
		}

		// reach the end of file
		if (pkt.eof == 0) {
			pkt.eof = 1;
			memset(&pkt.data, 0, sizeof(pkt.data));
			for (int i = 0; i < SEND_TIME; i++) {
				if(sendto(s, (void*) &pkt, sizeof(pkt), 0, (struct sockaddr*) &sin, sizeof(sin)) < 0)
					perror("sendto");
				usleep(1);
			}
		}

		close(fr);

		file_no++;
		seq_no = 0;
		
		memset(filepath, 0, sizeof(filepath));
	}

	close(s);
}

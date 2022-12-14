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
	// timeout
	struct timeval t = {0, 1};
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&t, sizeof(struct timeval));

	// check for ack every 1024 us
	//signal(SIGALRM, recv_ack);
	//ualarm( (useconds_t)( CHECK_ACK_TIME * 4 ), (useconds_t) CHECK_ACK_TIME * 4 );

	for (file_no = 0, seq_no = 0; file_no < nfiles; ) {
		
		//if(file_no>10) break;
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
			for (int i = 0; i < SEND_TIME; i++) {
				if(sendto(s, (void*) &pkt, sizeof(pkt), 0, (struct sockaddr*) &sin, sizeof(sin)) < 0)
					perror("sendto");
                // recv ack
                usleep(50);
				int rlen;
                if((rlen = recvfrom(s, (void*) &ack, sizeof(ack), MSG_DONTWAIT, NULL, NULL)) > 0){
					//printf("RECV: ");
					//printack(&ack);
					if(ack.file_no == pkt.file_no && ack.seq_no == pkt.seq_no+1){
                        break;
                    }
                }
				usleep(80);
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
				usleep(50);
                int rlen;
                if((rlen = recvfrom(s, (void*) &ack, sizeof(ack), MSG_DONTWAIT, NULL, NULL)) > 0){
					//printf("RECV: ");
					//printack(&ack);
					if(ack.file_no == pkt.file_no+1){
                        break;
                    }
                }
				usleep(80);
			}
		}

		close(fr);

		file_no++;
		seq_no = 0;
		
		memset(filepath, 0, sizeof(filepath));
	}

	sleep(8);
	close(s);
	exit(0);
}
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include "helpers.h"


int main(int argc, char** argv) {
	int clients_nr = 0;

	int udp_sock = socket(PF_INET, SOCK_DGRAM, 0);
	int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);

	DIE(udp_sock < 0 || tcp_sock < 0, "socket");

	struct sockaddr_in serv_addr;

	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[1]));
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	int tcp_bnd  = bind(tcp_sock, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
	DIE(tcp_bnd < 0, "bind");

	int tcp_lsn = listen(tcp_sock, ++clients_nr);
	DIE(tcp_lsn < 0, "listen");

	fd_set file_descr;
	FD_ZERO(&file_descr);

	FD_SET(tcp_sock, &file_descr);
	FD_SET(udp_sock, &file_descr);
	FD_SET(STDIN_FILENO, &file_descr);

	int max = 0;
	if(tcp_sock > udp_sock)
		max = tcp_sock;
	else max = udp_sock;

	while(1) {
		fd_set aux_set = file_descr;

		int sel = select(max + 1, &aux_set, NULL, NULL, NULL);
		DIE(sel < 0, "select");

		for (int i = 0; i < max; i++) {
			if (FD_ISSET(i, &aux_set)) {
				if (i == tcp_sock) {

				} else if (i == udp_sock) {

				} else if (i == STDIN_FILENO) {

				} else {

				}
			}
		}
	}

	close(udp_sock);
	close(tcp_sock);

	return 0;
}
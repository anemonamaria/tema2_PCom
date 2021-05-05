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

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	int clients_nr = 0;
	struct Packet *clients = calloc(1000, sizeof(struct Packet));

	int udp_sock = socket(PF_INET, SOCK_DGRAM, 0);
	int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);

	DIE(udp_sock < 0 || tcp_sock < 0, "socket");

	struct sockaddr_in serv_addr, udp_addr, new_tcp;

	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[1]));
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	int tcp_bnd  = bind(tcp_sock, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
	DIE(tcp_bnd < 0, "bind");

	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(atoi(argv[1]));
	udp_addr.sin_addr.s_addr = INADDR_ANY;

	int udp_bnd = bind(udp_sock, (struct sockaddr *)&udp_addr, sizeof(struct sockaddr));
	DIE(udp_bnd < 0, "bind");

	int tcp_lsn = listen(tcp_sock, ++clients_nr);
	DIE(tcp_lsn < 0, "listen");

	fd_set file_descr;
	FD_ZERO(&file_descr);

	FD_SET(tcp_sock, &file_descr);
	FD_SET(udp_sock, &file_descr);
	FD_SET(STDIN_FILENO, &file_descr);

	socklen_t udp_len = sizeof(struct sockaddr);
	int on = 0;
	struct online *online = calloc(100, sizeof(struct online));

	int max = 0;
	if(tcp_sock > udp_sock)
		max = tcp_sock;
	else max = udp_sock;

	while(1) {
		fd_set aux_set = file_descr;

		int sel = select(max + 1, &aux_set, NULL, NULL, NULL);
		DIE(sel < 0, "select");

		char buffer[100];
		memset(buffer, 0, 100);
		for (int i = 0; i <= max; i++) {
			if (FD_ISSET(i, &aux_set)) {
				if (i == tcp_sock) {
					int socket = accept(tcp_sock, (struct sockaddr *) &new_tcp, &udp_len);
					DIE(socket < 0, "accept");

					FD_SET(socket, &file_descr);
					if(socket > max)
						max = socket;

					int ret = recv(socket, buffer, 100, 0);
					DIE(ret < 0, "recv");

					for(int i = 5; i < 500; i++) {
						if(strcmp(clients[i].ip, buffer) == 0) {
							strcpy(buffer, "You are already connected.");
							int ret = send(socket, buffer, strlen(buffer) + 1, 0);
							DIE(ret < 0, "Msg not sent.\n");
							break;
						}
					}

					strcpy(clients[socket].ip, buffer);
					printf("New client %s connected from %s:%d.\n", buffer,
						inet_ntoa(new_tcp.sin_addr), ntohs(new_tcp.sin_port));


				} else if (i == udp_sock) {
					int ret = recvfrom(udp_sock, buffer, 1551, 0, (struct sockaddr *)&udp_addr,
						&udp_len);
					DIE(ret < 0, "revfrom");

					struct msg_tcp send_to_tcp;
					struct msg_udp *send_to_udp;
					send_to_tcp.port = htons(udp_addr.sin_port);
					strcpy(send_to_tcp.ip, inet_ntoa(udp_addr.sin_addr));

					send_to_udp = (struct msg_udp *)buffer;

					// completam mesajul TCP in functie de udp
					strcpy(send_to_tcp.topic, send_to_udp->topic);
					send_to_tcp.topic[50] = 0;

					long long num;
					double real;


					if(send_to_udp->type == 0) {
						num = ntohl(*(uint32_t *)(send_to_udp->continut + 1));

						if(send_to_udp->continut[0] == 1)
							num = num * (-1);

						strcpy(send_to_tcp.type, "INT");
					} else if (send_to_udp->type == 1) {
						real = ntohs(*(uint16_t *)(send_to_udp->continut));
						real = real / 100;
						sprintf(send_to_tcp.continut, "%.2f", real);
						strcpy(send_to_tcp.type, "SHORT_REAL");
					} else if (send_to_udp->type == 2) {
						int i, n = 1;
						real = ntohl(*(uint32_t *)(send_to_udp->continut + 1));

						for(i = 0; i < send_to_udp->continut[5]; i++)
							n = n * 10;

						real = real/n;

						if(send_to_udp->continut[0] == 1)
							real = real * (-1);

						strcpy(send_to_tcp.type, "FLOAT");
					} else {
						strcpy(send_to_tcp.type, "STRING");
						strcpy(send_to_tcp.continut, send_to_udp->continut);
					}

					for(int i = 0; i < on; i++) {
						if (strcmp(online[i].topic, send_to_tcp.topic) == 0) {
							for (int j = 5; j < 500; j++) {
								if (online[i].sockets[j] >= 5) {
									int ret = send(j, (char *) &send_to_tcp,
									 sizeof(struct msg_tcp), 0);
									DIE(ret < 0, "send");
								}
							}
							break;
						}
					}
				} else if (i == STDIN_FILENO) {
					fgets(buffer, 100, stdin);

					if(strncmp(buffer, "exit", 4) == 0)
						break;

				} else {
					struct Packet *input = (struct Packet *) buffer;

					if(strncmp(input->type,"s", 1) == 0) {
						int val = -1;
						for(int i = 0; i < on; i++) {
							if (strcmp(online[i].topic, input->topic) == 0) {
								val = i;
								break;
							}
						}
						if(val < 0 )
						{
							strcpy(online[on].topic, input->topic);
							online[on].sockets[i] = i;
							on++;
						} else {
							online[val].sockets[i] = i;
						}
					} else {
						int val = -1;
						for(int i = 0; i < on; i++) {
							if (strcmp(online[i].topic, input->topic) == 0) {
								val = i;
								break;
							}
						}
						if (val != -1) {
							int subscr = -1;
							for (int i = 5; i < 500; i++) {
								if(online[val].sockets[i] == i)
									subscr = i;
							}
							if(subscr != -1)
								online[val].sockets[subscr] = -1;
						}
					}
				}
			}
		}
	}


	close(udp_sock);
	close(tcp_sock);

	return 0;
}
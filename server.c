#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <limits.h>
#include "helpers.h"


int main(int argc, char** argv) {

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	DIE(argc < 2, "arguments");

	int enable = 1;
	struct client *clients = calloc(1000, sizeof(struct client));

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

	setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, &enable, 4);

	int tcp_lsn = listen(tcp_sock, INT_MAX);
	DIE(tcp_lsn < 0, "listen");

	fd_set file_descr;
	FD_ZERO(&file_descr);

	FD_SET(tcp_sock, &file_descr);
	FD_SET(udp_sock, &file_descr);
	FD_SET(STDIN_FILENO, &file_descr);

	socklen_t udp_len = sizeof(struct sockaddr);

	int max = 0;
	if(tcp_sock > udp_sock)
		max = tcp_sock;
	else max = udp_sock;

	int my_exit = 0;
	while(!my_exit) {
		fd_set aux_set = file_descr;

		int sel = select(max + 1, &aux_set, NULL, NULL, NULL);
		DIE(sel < 0, "select");

		char buffer[PACKLEN];

		for (int i = 0; i <= max; i++) {
			if (FD_ISSET(i, &aux_set)) {
				memset(buffer, 0, PACKLEN);
				if (i == tcp_sock) {
					int socket = accept(tcp_sock, (struct sockaddr *) &new_tcp, &udp_len);
					DIE(socket < 0, "accept");

					int ret = recv(socket, buffer, 10, 0);
					DIE(ret < 0, "recv");
					int found = -1, online = 0;
					for(int j = 5; j <= max; j++) {
						if(strcmp(clients[j].id, buffer) == 0) {
							found = j;
							online = clients[j].online;
							break;
						}
					}
					if(found == -1) {
						// ! client nou
						FD_SET(socket, &file_descr);
						if(socket > max)
							max = socket;

						strcpy(clients[max].id, buffer);
						clients[max].socket = socket;
						clients[max].online = 1;
						printf("New client %s connected from %s:%d\n", clients[max].id,
							inet_ntoa(new_tcp.sin_addr), ntohs(new_tcp.sin_port));
					} else if(found && !online) {
						FD_SET(socket, &file_descr);
						clients[found].socket = socket;
						clients[found].online = 1;
						printf("New client %s connected from %s:%d.\n", clients[found].id,
							inet_ntoa(new_tcp.sin_addr), ntohs(new_tcp.sin_port));
						for(int k = 0; k < clients[found].dim_unsent; k++){
							int ret = send(clients[found].socket, &clients[found].unsent[k],
										sizeof(struct tcp_struct), 0);
							DIE(ret < 0, "send");
						}
						clients[found].dim_unsent = 0;
					} else {
						// ! client nou cu id vechi
						close(socket);
						printf("Client %s already connected.\n", clients[found].id);
					}

				} else if (i == udp_sock) {
					int ret = recvfrom(udp_sock, buffer, 1551, 0, (struct sockaddr *)&udp_addr,
						&udp_len);
					DIE(ret < 0, "revfrom");

					struct tcp_struct send_to_tcp;
					memset(&send_to_tcp, 0, sizeof(struct tcp_struct));
					struct udp_struct *send_to_udp;

					send_to_tcp.port = htons(udp_addr.sin_port);
					strcpy(send_to_tcp.ip, inet_ntoa(udp_addr.sin_addr));

					send_to_udp = (struct udp_struct *)buffer;

					strcpy(send_to_tcp.topic, send_to_udp->topic);
					send_to_tcp.topic[50] = 0;

					uint32_t num;
					double real;


					if(send_to_udp->type == 0) {
						num = ntohl(*(uint32_t *)(send_to_udp->continut + 1));
						if(send_to_udp->continut[0] == 1) {
							num = num * (-1);
							sprintf(send_to_tcp.continut, "%d", num);
						} else {
							sprintf(send_to_tcp.continut, "%d", num);
						}
						strcpy(send_to_tcp.type, "INT");
					} else if (send_to_udp->type == 1) {
						real = abs(ntohs(*(uint16_t *)(send_to_udp->continut)));
						real = real / 100;
						strcpy(send_to_tcp.type, "SHORT_REAL");
						sprintf(send_to_tcp.continut, "%.2f", real);
					} else if (send_to_udp->type == 2) {
						int j, n = 1;
						real = ntohl(*(uint32_t *)(send_to_udp->continut + 1));

						for(j = 0; j < send_to_udp->continut[5]; j++)
							n = n * 10;

						real = real/n;
						strcpy(send_to_tcp.type, "FLOAT");

						if(send_to_udp->continut[0] == 1) {
							real = real * (-1);
							sprintf(send_to_tcp.continut, "%lf", real);
						} else {
							sprintf(send_to_tcp.continut, "%lf", real);
						}
					} else {
						strcpy(send_to_tcp.type, "STRING");
						strcpy(send_to_tcp.continut, send_to_udp->continut);
					}

					for(int j = 5; j <= max; j++) {
						for(int k = 0; k < clients[j].dim_topics; k++) {
							if (strcmp(clients[j].topics[k].nume, send_to_tcp.topic) == 0) {
								if(clients[j].online){
									int ret = send(clients[j].socket, &send_to_tcp,
										sizeof(struct tcp_struct), 0);
									DIE(ret < 0, "send");
								} else {
									if(clients[j].topics[k].sf == 1) {
										clients[j].unsent[clients[j].dim_unsent++] = send_to_tcp;
									}
								}
								break;
							}
						}
					}
				} else if (i == STDIN_FILENO) {
					fgets(buffer, 100, stdin);

					if(strncmp(buffer, "exit", 4) == 0) {
						my_exit = 1;
						break;
					}

					DIE(strncmp(buffer, "exit", 4) != 0, "exit");

				} else {
					memset(buffer, 0, PACKLEN);
					int ret = recv(i, buffer, PACKLEN, 0);
					DIE(ret < 0, "recv");
					if(ret) {
						struct Packet *input = (struct Packet *) buffer;
						client* c = NULL;

						for(int j = 5; j <= max; j++) {
							if (i == clients[j].socket) {
								c = &clients[j];
								break;
							}
						}

						if(input->type == 's') {
							int topicIndex = -1;

							for(int k = 0; k < c->dim_topics; k++) {
								if (strcmp(c->topics[k].nume, input->topic) == 0) {
									topicIndex = k;
									break;
								}
							}

							if(topicIndex < 0) {
								strcpy(c->topics[c->dim_topics].nume, input->topic);
								c->topics[c->dim_topics].sf = input->tip_date;
								c->dim_topics++;
							}
						} else if(input->type == 'u') {
							int topicIndex = -1;
							for(int k = 0; k < c->dim_topics; k++)
								if(strcmp(c->topics[k].nume, input->topic) == 0) {
									topicIndex = k;
									break;
								}
							if (topicIndex >= 0) {
								for(int l = topicIndex; l < c->dim_topics; l++)
									c->topics[l] = c->topics[l+1];
								c->dim_topics--;
							}
						} else if (input->type == 'e') {
							for (int j = 5; j <= max; j++) {
								if(clients[j].socket == i) {
									printf("Client %s disconnected.\n", clients[j].id);
									clients[j].online = 0;
									clients[j].socket = -1;
									FD_CLR(i, &file_descr);
									close(i);
									break;
								}
							}
						}
					} else if (ret == 0) {
						for (int j = 5; j <= max; j++) {
							if(clients[j].socket == i) {
								printf("Client %s disconnected.\n", clients[j].id);
								clients[j].online = 0;
								clients[j].socket = -1;
								FD_CLR(i, &file_descr);
								close(i);
								break;
							}
						}
					}
				}
			}
		}
	}
	for(int i = 3; i <= max; i++) {
		if(FD_ISSET(i, &file_descr))
			close(i);
	}

	return 0;
}


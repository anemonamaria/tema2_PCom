#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include "helpers.h"


int main(int argc, char** argv) {

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	DIE(argc < 4, "arguments");

	int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_sock < 0, "socket");

	struct sockaddr_in server_data;
	server_data.sin_family = AF_INET;
	server_data.sin_port = htons(atoi(argv[3]));
	inet_aton(argv[2], &server_data.sin_addr);

	fd_set file_descr;
	FD_ZERO(&file_descr);

	FD_SET(tcp_sock, &file_descr);
	FD_SET(STDIN_FILENO, &file_descr);

	int	ret2 = connect(tcp_sock, (struct sockaddr *)&server_data, sizeof(server_data));
	DIE(ret2 < 0, "connect");

	int ret = send(tcp_sock, argv[1], 10, 0);
	DIE(ret < 0, "send");

	struct Packet pack;
	int enable = 1;
	setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&enable, sizeof(int));
	while(1) {
		fd_set aux_set = file_descr;

		int sel = select(tcp_sock + 1, &aux_set, NULL, NULL, NULL);
		DIE(sel < 0, "select");
		//comanda la stdin
		if (FD_ISSET(STDIN_FILENO, &aux_set)) {
			char buffer[100];
			memset(buffer, 0, 100);
			fgets(buffer, 100, stdin);
			memset(&pack, 0, PACKLEN);

			if (strncmp(buffer, "exit", 4) == 0) {
				pack.type = 'e';
				int sending = send(tcp_sock, &pack, PACKLEN, 0);
				DIE (sending < 0, "send");
				break;
			}

			else if (strncmp(buffer, "subscribe", 9) == 0) {
				char *token = strtok(buffer, " ");
				pack.type = 's';
				token = strtok(NULL, " ");
				strcpy(pack.topic, token);
				token = strtok(NULL, " ");
				pack.tip_date = token[0] - '0';

				int sending = send(tcp_sock, &pack, PACKLEN, 0);
				DIE (sending < 0, "send");
				printf("Subscribed to topic.\n");
			}

			else if (strncmp(buffer, "unsubscribe", 11) == 0) {
				char *token = strtok(buffer, " ");
				strcpy(&pack.type, "u");
				token = strtok(NULL, " ");
				strcpy(pack.topic, token);
				token = strtok(NULL, " ");
				pack.tip_date = token[0];

				int sending = send(tcp_sock, &pack, PACKLEN, 0);
				DIE (sending < 0, "send");
				printf("Unsubscribed to topic.\n");
			}
			else printf("Invalid cmd.\n");
		}
		// mesajul de la server
		if(FD_ISSET(tcp_sock, &aux_set)) {
			char buffer[sizeof(struct tcp_struct)];
			memset(buffer, 0, sizeof(struct tcp_struct));

			int ret = recv(tcp_sock, buffer, sizeof(struct tcp_struct), 0);
			DIE(ret < 0, "receive");

			if(ret == 0) break;
			struct tcp_struct *pack_send = (struct tcp_struct *)buffer;
			printf("%s:%u - %s - %s - %s\n", pack_send->ip, pack_send->port,
				pack_send->topic, pack_send->type, pack_send->continut);
		}
	}

	close(tcp_sock);
	return 0;
}


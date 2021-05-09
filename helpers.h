#ifndef _HELPERS_H
#define _HELPERS_H 1

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)


typedef struct Packet {
	char type; // exit = e, subscribe = s, unsubscribe = u
	char topic[51];
	char tip_date;
	char continut[1501];
	char ip[16];
	uint16_t port;
	int sf;
} Packet;


#define PACKLEN sizeof(struct Packet)


//structura pentru clinetii abonati
struct online {
	char topic[50];
	int sockets[500];
} online;

//structura pentru mesaj TCP
typedef struct msg_tcp {
	char ip[16];
	uint16_t port;
	char type[11];
	char topic[51];
	char continut[1501];
} msg_tcp;

//structura pentru mesaj UDP
typedef struct msg_udp {
	char topic[50];
	uint8_t type;
	char continut[1501];
} msg_udp;

typedef struct topic{
	char nume[51];
	int sf;
} topic;

typedef struct client{
	char id[10];
	int socket;
	int dim_topics;
	int dim_unsent;
	struct msg_tcp unsent[100];
	struct topic topics[100];
	int online; // 1 da 0 nu
} client;


#endif













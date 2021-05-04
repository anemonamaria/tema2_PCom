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
	char topic[50];
	char tip_date;
	char continut[1500];
	char ip[16];
	uint16_t port;
} Packet;


#endif
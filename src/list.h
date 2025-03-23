#include <netinet/in.h>
#include "buffer.h"
#ifndef LL
#define LL

typedef struct ClientNode {
	int fd;
	struct ClientNode *next;
	char ipstr[INET_ADDRSTRLEN];
	char nickname[LENGTH_NAME];
} ClientNode;

ClientNode* new_node(int sockfd, const char* ipstr);

void add_node(ClientNode* client);

void remove_node(ClientNode* client);

void delete_list(ClientNode* head);

#endif
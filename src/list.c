#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "list.h"

ClientNode* head;

ClientNode* new_node(int sockfd, const char* ipstr){
	ClientNode* client = (ClientNode*) malloc(sizeof(ClientNode));
	client->fd=sockfd;
	client->next=NULL;
	strcpy(client->ipstr,ipstr);
	strcpy(client->nickname,"\0");
	return client;
}

void add_node(ClientNode* client){
	if (!head){
		head=client;
	} else {
		ClientNode *tmp;
		for (tmp = head; tmp->next; tmp=tmp->next);
		tmp->next=client;
	}
}

void remove_node(ClientNode* client){
	if (head==client) {
		head=head->next;
		close(client->fd);
		free(client);
	} else {
		for (ClientNode *tmp = head; tmp->next; tmp=tmp->next) {
			if (tmp->next == client){
				tmp->next = client->next;
				close(client->fd);
				free(client);
				break;
			}
		}		
	}
}

void delete_list(ClientNode* head){
	ClientNode *tmp;
	while (head){
		tmp = head;
		close(head->fd);
		head = head->next;
		free(tmp);
	}
}


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "list.h"

#define PORT "8274"

extern ClientNode* head;
int server_sockfd;

void cleanup(){
	printf("\nshutting down\n");
	close(server_sockfd); //close listener socket
	delete_list(head); //close remaining connected client sockets
	exit(0);
}

void send_to_clients(ClientNode* client, char *buf){
	for (ClientNode *tmp = head; tmp; tmp=tmp->next) {
		if (tmp->fd==client->fd) continue;
		send(tmp->fd,buf,LENGTH_OUTGOING,0);
	}
}

void* client_handler(void* arg){
	ClientNode *client=(ClientNode*)arg;
	char buf[LENGTH_OUTGOING];
	
	recv(client->fd,client->nickname,LENGTH_NAME,0);

	snprintf(buf,LENGTH_OUTGOING,"%s (%s) has joined",client->nickname,client->ipstr);
	printf("%s, socket %d\n", buf, client->fd);
	send_to_clients(client,buf);
	
	while (1){
		if (!recv(client->fd,buf,LENGTH_OUTGOING,0) || !strcmp(buf,"!!!")) {
			snprintf(buf,LENGTH_OUTGOING,"%s (%s) has left",client->nickname,client->ipstr);
			printf("%s, socket %d\n", buf, client->fd);
			send_to_clients(client,buf);
			break;
		} else if (!strcmp(buf,"ls")) {
			int client_count=0;
			char tbuf[LENGTH_OUTGOING]={0};
			for (ClientNode *tmp = head; tmp; tmp=tmp->next, client_count++) { //unsafe buffer overflow possibility
				strcat(tbuf,tmp->nickname);
				if (tmp->next) strcat(tbuf,",\n\t");
			}
			snprintf(buf,LENGTH_OUTGOING,"%d online:\n\t%s",client_count,tbuf);
			send(client->fd,buf,LENGTH_OUTGOING,0);
		}
		else if (strlen(buf)==0) continue;
		else {
			char tbuf[LENGTH_OUTGOING];
			snprintf(tbuf,LENGTH_OUTGOING,"%s: %s",client->nickname,buf);
			send_to_clients(client,tbuf);
		}
	}

	remove_node(client);
	return NULL;
}

int main(){
	//setup listener socket -> accept connections, create thread per client
	signal(SIGINT,cleanup);

	int client_sockfd;
	struct addrinfo hints, *res, *tmp;
	struct sockaddr_in client_addr;
	int y=1;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family=AF_INET;
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_flags=AI_PASSIVE;

	getaddrinfo(NULL, PORT, &hints, &res);

	for (tmp = res; tmp; tmp=tmp->ai_next){
		//assign socket fd
		if ((server_sockfd=socket(tmp->ai_family,tmp->ai_socktype,tmp->ai_protocol)) == -1) {
			perror("could not assign socket fd");
			continue; //go to next addrinfo and reattempt
		}

		//allow binding to same local address (useful when restarting server)
		if (setsockopt(server_sockfd,SOL_SOCKET,SO_REUSEADDR,&y,sizeof(y)) == -1){
			perror("setsockopt");
			exit(1);
		}

		if (bind(server_sockfd,tmp->ai_addr,tmp->ai_addrlen) == -1){
			perror("could not bind");
			close(server_sockfd);
			continue; //reattempt
		}
		break;
	}
	
	if (!tmp){
		fprintf(stderr,"could not assign socketfd and bind");
		exit(1);
	}
	
	freeaddrinfo(res);

	listen(server_sockfd,5);
	printf("server ready to accept connections\n");
	fflush(stdout); //accept() is blocking, so flush buffer to print before calling

	while(1) {
		int addrlen = sizeof(client_addr);
		if ((client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_addr, (socklen_t*)&addrlen)) == -1) {
			perror("could not accept client");
			continue;
		}

		char ipstr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET,&client_addr.sin_addr,ipstr,INET_ADDRSTRLEN);
		printf("received connection from %s\n",ipstr);

		ClientNode *client = new_node(client_sockfd,ipstr);
		add_node(client);

		pthread_t tid;
		if (pthread_create(&tid,NULL,client_handler,client)) {
			perror("pthread not created");
			remove_node(client);
			continue;
		}
		pthread_detach(tid);
	}
}
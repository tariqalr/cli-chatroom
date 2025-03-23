#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "buffer.h"

#define PORT "8274"

int client_sockfd;

void recv_handler(){
	char buf[LENGTH_INCOMING];
	while (1){
		if (!recv(client_sockfd,buf,LENGTH_INCOMING,0)) break; //connection closed by server
		printf("\r%s\n> ", buf);
		fflush(stdout);
	}
}

void send_handler(){
	char buf[LENGTH_OUTGOING];
	while (1) {
		if (fgets(buf,LENGTH_OUTGOING,stdin)) {
			buf[strcspn(buf,"\n")]='\0';
			send(client_sockfd,buf,LENGTH_OUTGOING,0);
			printf("> ");
			fflush(stdout);
			if (!strcmp(buf,"!!!")) break;
		}
	}
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr,"usage: ./client <ip>");
		exit(1);
	}
	struct addrinfo hints, *res, *tmp;

	printf("Welcome\nSend !!! to exit, ls to display users\n");

	char nickname[LENGTH_NAME];
	while(1){
		printf("\nEnter nickname: ");
		if (!fgets(nickname,LENGTH_NAME,stdin)) {
			perror("fgets");
			continue;
		}
		nickname[strcspn(nickname,"\n")]='\0';
		break;
	}

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family=AF_INET;
	hints.ai_socktype=SOCK_STREAM;
	
	getaddrinfo(argv[1], PORT, &hints, &res);

	for (tmp = res; tmp; tmp=tmp->ai_next){
		//assign socket fd
		if ((client_sockfd=socket(tmp->ai_family,tmp->ai_socktype,tmp->ai_protocol)) == -1) {
			perror("could not assign socket fd");
			continue; //go to next addrinfo and reattempt
		}

		if (connect(client_sockfd,tmp->ai_addr,tmp->ai_addrlen) == -1) {
			perror("could not connect to server");
			continue;
		}
		break;
	}
	
	if (!tmp){
		fprintf(stderr,"could not assign socketfd and connect");
		exit(1);
	}

	char ipstr[INET_ADDRSTRLEN];
	inet_ntop(AF_INET,&((struct sockaddr_in*)tmp->ai_addr)->sin_addr,ipstr,INET_ADDRSTRLEN);
	printf("\nconnecting to %s...\n",ipstr);
	
	freeaddrinfo(res);

	send(client_sockfd,nickname,LENGTH_NAME,0);
	printf("> ");

	pthread_t recv_tid;
	if (pthread_create(&recv_tid,NULL,(void*)recv_handler,NULL)) {
		perror("recv pthread not created");
	}

	pthread_t send_tid;
	if (pthread_create(&send_tid,NULL,(void*)send_handler,NULL)) {
		perror("send pthread not created");
	}

	pthread_join(send_tid,NULL);
	pthread_join(recv_tid,NULL);

	close(client_sockfd);
}
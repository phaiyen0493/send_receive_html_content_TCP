/*Name: Yen Pham
CS3530
Project 2- Create a web proxy server that can be connected by a single client 
and would only allow http requests. The proxy server should be able to cache 
up to five recent websites.
*/

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <signal.h>
#include <wchar.h>

#define PORT 11103

char message[99999];
//Receive message
void *receive_message(void *server_socket)
{
	int socket = *((int *) server_socket);
	int message_length;
	
	while ((message_length = recv(socket, message, 99999 , 0)) > 0 )
	{
		
		//message[message_length] = '\0';
		fputs(message, stdout); //display message
		printf("\n");
		//printf("\nlength=%d",message_length);
		memset(&message, 0, 99999); //clean buffer
	} 
	printf("\n");
	return NULL;
}

int main(int argc, char * argv[])
{
	pthread_t receive_thread;//create thread
	int message_length;
	int socketfd;
	char *message_buffer;
	struct sockaddr_in server_address;
	socketfd = socket( AF_INET, SOCK_STREAM,0);
	server_address.sin_port = htons(PORT);
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr("129.120.151.94");
	
	int sk_option = 1;
	//display if failed to send
	if( setsockopt(socketfd, IPPROTO_TCP, TCP_NODELAY, (char *) &sk_option, sizeof(sk_option)) < 0 ) { 
		perror("ERROR: setsockopt failed"); 
		return EXIT_FAILURE; 
	} 
  
	//connect the server
	if ((connect (socketfd, (struct sockaddr *) &server_address, sizeof(server_address))) == -1)
	{
		printf("Connection to socket failed \n");
		exit(EXIT_FAILURE);
	}

	printf("Client %d\n", PORT);
	//creating a client thread which is always waiting for a message
	pthread_create(&receive_thread, NULL, (void *) receive_message, &socketfd);
	
	message_buffer = malloc(sizeof(char)*512);
	//read message from console
	
	bool toCont = true;
	
	while ( toCont ) //continue until message is "quit"
	{	
		fgets(message, 512, stdin);
		//fputs(message, stdout);
		if ( strcmp(message, "\n") != 0 )
		{
			if ( strcmp(message, "quit\n") == 0 )
			{
				printf("Quitting\n");		
				toCont = false;
			}
		
			memset(message_buffer, 0, 512);	//reallocate memory
			strcpy(message_buffer, message);//copy message to message buffer
			message_length = send(socketfd, message_buffer, strlen(message_buffer), 0);//send message to server
			if (message_length < 0)
			{
				printf("Fail to send message\n");
			}
		}
	}

	pthread_join(receive_thread, NULL); //close thread

	close(socketfd);//close socket
	return 0;
}
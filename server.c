/*Name: Yen Pham
CS3530
Project 2- Create a web proxy server that can be connected by a single client 
and would only allow http requests. The proxy server should be able to cache 
up to five recent websites.
*/

#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h> 
#include <netdb.h>
#include <wchar.h>
#include <time.h> 

#define PORT 11103

struct client 
{				
	int sockfd;//file descriptor
	struct sockaddr_in loc;//connecting address, will be same for all the same ips if used in same machine
};

//to hold 100 maximum clients
struct client *client_list[100];

//to count the number of clients
int client_count = 0;

void socket_connect(char *url_string, int client_sockfd)
{
	//printf("url=%s\n",url_string);
	bool cache_check = false;
	char file_str[500];
	char file_time[32];
	FILE *fp;
	fp = fopen("list.txt", "r"); // read mode

	char cached_url_list[5][500];
	char cached_time_list[5][32];

	if (fp != NULL) //if list.txt file exists
	{
		fseek(fp, 0, SEEK_END); // goto end of file
		if (ftell(fp) != 0) // if list.txt file is not empty
		{
			fseek(fp, 0, SEEK_SET);
			int url_count = 0;
			while(fscanf(fp, "%99[^ ] %99[^\n]\n", file_str, file_time) != EOF)
			{
				//printf("%s\n", file_str);
				//printf("%s\n", file_time);

				strcpy(cached_url_list[url_count], file_str);//assign url string to array list
				strcpy(cached_time_list[url_count], file_time); //assign time string to array list
				strcat(file_str, "\n\0");

				if (strcmp(url_string, file_str) == 0)
				{
					cache_check = true;
					//get the file
					char file_name[15];
					sprintf(file_name, "%d", url_count); //convert from number to file name
					FILE *fp1;
					fp1 = fopen(file_name, "r"); // read mode
					if (fp1 == NULL)
					{
						perror("Error while opening the file.\n");
						exit(EXIT_FAILURE);
					}
					fseek(fp1, 0, SEEK_END); //get the number of bytes
					long numbytes = ftell(fp1); 
					fseek(fp1, 0, SEEK_SET); //reset the file position indicator to the beginning of the file 
					char file_buffer [numbytes + 1]; //grab sufficient memory for the buffer to hold the text
					fread(file_buffer, 1, numbytes+1, fp1); //copy all the text into the buffer
					file_buffer[numbytes] = '\0';
					write(client_sockfd, file_buffer, strlen(file_buffer));	
					//memset(&file_buffer, 0, numbytes + 1);
					fclose(fp1);
				}
				url_count++;
				memset(&file_str, 0, 500);
				memset(&file_time, 0, 32);
			}
		}
		fclose(fp);
	}

	if (cache_check == false)
	{
		//Extract domain and path name from url
		char new_url_string[500];
		strcpy (new_url_string, url_string);//will use it later for saving cached files
		char *domain = malloc (sizeof(char)*512);
		char *path = malloc (sizeof(char)*512);
		memset(domain, 0, 512);
		memset(path, 0, 512);

		int backslash_count;
		if (url_string[0] == 'h' && url_string[1] == 't' && url_string[2] == 't' && url_string[3] == 'p' && url_string[4] == ':')
		{
			backslash_count = 0;
			for (int i = 0; url_string[i] != '\0'; i++)
			{
				if (url_string[i] =='/')
				{
					backslash_count++; //count number of back slash to identify the path
				}
			}
		
			if (backslash_count > 2)
			{
				sscanf(url_string, "http://%99[^/]/%99[^\n]", domain, path); //extract domain and path from http url
			}
			else if (backslash_count == 2)
			{
				sscanf(url_string, "http://%99[^\n]", domain);
			}
		}
		else if(url_string[0] == 'w' && url_string[1] == 'w' && url_string[2] == 'w' && url_string[3] == '.')
		{
			backslash_count = 0;
			for (int i = 0; url_string[i] != '\0'; i++)
			{
				if (url_string[i] =='/')
				{
					backslash_count++;
				}
			}
		
			if (backslash_count > 0)
			{
				sscanf(url_string, "%99[^/]/%99[^\n]", domain, path);
			}
			else if (backslash_count == 0)
			{
				sscanf(url_string, "%99[^\n]", domain);
			}
		}
		else if (strlen(url_string) > 0)//in case url does not start with http or www
		{
			char *temp = malloc (sizeof(char)*512);
			memset(temp, 0, 512);
			strcpy(domain, "www.");
			backslash_count = 0;
			for (int i = 0; url_string[i] != '\0'; i++)
			{
				if (url_string[i] =='/')
				{
					backslash_count++;
				}
			}
		
			if (backslash_count > 0)
			{
				sscanf(url_string, "%99[^/]/%99[^\n]", temp, path);
			}
			else if (backslash_count == 0)
			{
				sscanf(url_string, "%99[^\n]", temp);
			}

			strcat(domain, temp);
		}

		//printf("%s\n",domain);
		//printf("path=%s\n", path);

		//Stream sockets and rcv()
		struct addrinfo hints, *res;
		int web_server_sockfd;

		char buf[99999];
		long byte_count;

		//get host info, make socket and connect it
		memset(&hints, 0,sizeof (hints));
		hints.ai_family=AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		getaddrinfo(domain,"80", &hints, &res);
		web_server_sockfd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
		connect(web_server_sockfd,res->ai_addr,res->ai_addrlen);
	
		char *header = malloc (sizeof(char)*512);
	
		if (strlen(path) != 0)
		{
			sprintf(header, "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", path, domain);
		}
		else
		{
			sprintf(header, "GET / HTTP/1.1\r\nHost: %s\r\n\r\n", domain);
		}

		//printf("path=%s\n", path);
		//printf("header=%s\n",header);
		send(web_server_sockfd,header,strlen(header),0); //send header to web server
		memset(&buf, 0, 99999);	
		byte_count = recv(web_server_sockfd, buf, sizeof (buf) - 1, 0);
		//printf("%s\n",buf);
		//printf("%d\n", byte_count);
		write(client_sockfd, buf, strlen(buf));

		char http_status[4];
		int c = 0;
		while (c < 3) 
		{
			http_status[c] = buf[9+c]; //extract http status from the response
			c++;
		}
		http_status[c] = '\0';
		//printf("%s\n",http_status);
		 
		if (strcmp(http_status, "200") == 0) //cache if http response is 200
		{
			new_url_string[strlen(new_url_string)-1] = '\0';
			//get time stamp
			time_t seconds;
			time(&seconds);
			struct tm* access_time = localtime(&seconds);
			char access_time_buffer[32];
			int year, month, day, hour, minute, second;
			hour = access_time->tm_hour;
			minute = access_time->tm_min;
			second = access_time->tm_sec;
			day = access_time->tm_mday;
			month = access_time->tm_mon + 1;
			year = access_time -> tm_year + 1900;
			sprintf(access_time_buffer, "%d%02d%02d%02d%02d%02d", year, month, day, hour, minute, second);
			char new_file_name[15];	
			bool empty_slot_check = false;	

			for (int i = 0; i < 5; i++)
			{
				if (cached_url_list[i][0] == (char)0 && cached_time_list[i][0] == (char)0)
				{
					//add to the empty slot
					empty_slot_check = true;
					strcpy(cached_url_list[i], new_url_string);
					strcpy(cached_time_list[i], access_time_buffer);
					sprintf(new_file_name, "%d", i); //convert from number to file name
					break;
				}
			}

			if (empty_slot_check == false)
			{
				//identify the oldest time in the cach list
				struct tm oldest_time;
				sscanf(cached_time_list[0], "%04d%02d%02d%02d%02d%02d[^\n]", &year, &month, &day, &hour, &minute, &second);
				oldest_time.tm_year = year - 1900;
				oldest_time.tm_mon = month - 1;
				oldest_time.tm_mday = day;
				oldest_time.tm_hour = hour;
				oldest_time.tm_min = minute;
				oldest_time.tm_sec = second;
				time_t oldest_time_seconds = mktime(&oldest_time);

				int position_for_replacement = 0;
				for (int k = 0; k < 5; k++)
				{
					sscanf(cached_time_list[k], "%04d%02d%02d%02d%02d%02d[^\n]", &year, &month, &day, &hour, &minute, &second);
					oldest_time.tm_year = year - 1900;
					oldest_time.tm_mon = month - 1;
					oldest_time.tm_mday = day;
					oldest_time.tm_hour = hour;
					oldest_time.tm_min = minute;
					oldest_time.tm_sec = second;
					time_t converted_time_seconds = mktime(&oldest_time);
					if (converted_time_seconds < oldest_time_seconds)
					{
						oldest_time_seconds = converted_time_seconds;
						position_for_replacement = k;
					}
				}
				
				strcpy(cached_url_list[position_for_replacement], new_url_string);
				strcpy(cached_time_list[position_for_replacement], access_time_buffer);
				sprintf(new_file_name, "%d", position_for_replacement); //convert from number to file name
			}
			FILE *fp2;
			fp2 = fopen(new_file_name, "w");
			fwrite(buf, 1, sizeof(buf), fp2); //write buffer info to the file
			fclose(fp2);

			FILE *fp3 = fopen("list.txt", "w");//reopen the list.txt file and write new data
			for (int g = 0; g < 5; g++)
			{
				if (cached_url_list[g][0] != (char)0 && cached_time_list[g][0] != (char)0)
				{
					fprintf(fp3, "%s %s\n", cached_url_list[g], cached_time_list[g]);
				}
			}
			fclose(fp3);
		}
		memset(&new_url_string,0 , 500); 
	}
	return;
}

void remove_client(int sockfd)
{
	for (int i = 0; i < client_count; i++)
	{
		if (client_list[i] != NULL && client_list[i]->sockfd == sockfd)
		{
			client_list[i] = NULL; //reset to null to remove the client
			client_count--;
		}
	}
}

void *server_handler(void *c)
{
	char *message = malloc (sizeof(char)*512);
	struct client *temp_client = (struct client *) c;
	int rec_bytes;
	bool toContinue = true;	

	while (toContinue)
	{
		memset(message, 0, 512);//reset buffer memory
		rec_bytes = recv(temp_client->sockfd, message, 512, 0);//receive message from client
		//printf("rec_bytes=%d\n",rec_bytes);
		
		if (rec_bytes > 0 && strcmp(message, "\n") != 0)
		{
			if (strcmp(message, "quit\n") != 0)
			{
				//fputs(message, stdout);
				//printf("b1\n");
				socket_connect(message, temp_client->sockfd); //handle proxy server			
			}
			else
			{
				toContinue = false;
				printf("Client at socket #%d quitted\n", temp_client->sockfd);	
			}
		}
		else if (rec_bytes == 0)
		{
			toContinue = false;
			printf("Client at socket #%d quitted\n", temp_client->sockfd);	
		}
	}

	remove_client(temp_client->sockfd);
	close(temp_client->sockfd);
	free(temp_client); //free temporary struct client
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	int listen_fd, conn_fd;
	pthread_t recvt[100];
	struct sockaddr_in servaddr;

	// An client struct of sockaddr_in
	struct sockaddr_in single_client;

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);//listen to socket
	if (listen_fd == -1)
	{
		perror("Cannot listen to socket\n");
		exit(EXIT_FAILURE);
	}

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);

	//to make sure it can use that some port later too
	int on = 1; 
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	if (bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)))
	{
		perror("Bind error\n");
		exit(EXIT_FAILURE);
	}

	if (listen(listen_fd, 10) == -1)
	{
		perror("Listen error\n");
		exit(EXIT_FAILURE);
	}

	printf("wcserver %d\n", PORT);

	int g = 0;
	while(1)
	{
		int length = sizeof(single_client);
		conn_fd = accept(listen_fd, (struct sockaddr*) &single_client, &length);//connect to client

		if (client_count>=100) //reject if too many client get in
		{
			printf("Too many clients already! Connecion Rejected\n");
			continue;
		}

		struct client *temp_client;
		temp_client = malloc(sizeof(struct client));

		temp_client->loc = single_client;//ip address
		temp_client->sockfd = conn_fd;//socket number

		int i = 0;
		while(1)
		{
			if(client_list[i] == NULL)
			{
				client_list[i] = temp_client; //add client to array
				break;
			}
			i++;
		}
		pthread_create(&recvt[g++], NULL, (void*) &server_handler, (void*)temp_client);
		client_count++;	
	}

 	for (int n = 0 ; n < g; n++)
	{
		pthread_detach(recvt[n]);
	}

	return 0;
}

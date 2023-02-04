#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#define BUFFER_SIZE 1024
#define MAX_CONNECTION 10

static inline void check_result(int r, char *string)
{
	if(r == -1)
	{
		printf("%s failed\n", string);
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	int r, sockfd, clientfd, fd;
	struct addrinfo hints, *host_server;
	struct sockaddr client_addr;
	socklen_t client_len;
	fd_set main_fdset, read_fdset;
	char connection_list[MAX_CONNECTION+1][BUFFER_SIZE];

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	char *server_ip = "127.0.0.1";
	char *port = "65001";
	int backlog = MAX_CONNECTION;
	short done = 0;

	r = getaddrinfo(server_ip, port, &hints, &host_server);

	if(r != 0)
	{
		perror("Failed getaddrinfo()");
		exit(1);
	}
	
	sockfd = socket(
		host_server->ai_family,
		host_server->ai_socktype,
		host_server->ai_protocol
		);

	check_result(sockfd, "socket");

	r = bind(sockfd, host_server->ai_addr, host_server->ai_addrlen);

	check_result(r, "bind");

	r = listen(sockfd, backlog);

	check_result(r, "listen");

	FD_ZERO(&main_fdset);
	FD_SET(sockfd, &main_fdset);
	FD_SET(1, &main_fdset);
	strcpy(connection_list[sockfd],server_ip);

//	printf("Outside While");
	while(!done)
	{	
		read_fdset = main_fdset;

		r = select(MAX_CONNECTION+1, &read_fdset, NULL, NULL, 0);

		check_result(r, "select");
		
		printf("SERVER>>");

		for(fd = 1; fd <= MAX_CONNECTION; fd++)
		{
			printf("fd = %d\n",fd);
			if(FD_ISSET(fd, &read_fdset))
			{

				printf("fd %d is set\n", fd);
				if(fd == sockfd)
				{
					printf("New connection sockfd %d\n",sockfd);

					client_len = sizeof(struct sockaddr);
					clientfd = accept(sockfd, &client_addr, &client_len);
					printf("Client fd = %d\n",clientfd);
					check_result(r, "accept");

					FD_SET(clientfd, &main_fdset);

					char hostname[BUFFER_SIZE];
					r = getnameinfo(&client_addr, client_len, hostname, BUFFER_SIZE, 0, 0, NI_NUMERICHOST);

					check_result(r, "getnameinfo");
					
					strcpy(connection_list[clientfd],hostname);

					char buffer_client[BUFFER_SIZE];

					strcpy(buffer_client, "SERVER>> Welcome to the chatroom ");
					strcat(buffer_client, hostname);
					strcat(buffer_client, "!!\n");
					strcat(buffer_client, "SERVER>> Type close to disconnect, shutdown to stop\n");

					char buffer_all[BUFFER_SIZE];
					strcpy(buffer_all, hostname);
					strcat(buffer_all, " has joined the server");

					send(clientfd, buffer_client, strlen(buffer_client), 0);
					
					for(int i=sockfd+1; i <= MAX_CONNECTION; i++)
					{
						printf("i = %d\t fd = %d\n", i, fd);
						if(i!=clientfd && FD_ISSET(i, &main_fdset))
						{
							printf("Inside i = %d\tsockfd = %d\n", i, sockfd);
							send(i, buffer_all, strlen(buffer_all), 0);
						}	
					}

					printf("Reached end of if case\n");
				}
				else
				{

					printf("Receiving data from fd %d\n", fd);
					char recv_buffer[BUFFER_SIZE];
					char buffer_all[BUFFER_SIZE];
					
					r = recv(fd, recv_buffer, BUFFER_SIZE, 0);
					
					if(r < 1)
					{
						perror("Failed, nothing received from client");

						FD_CLR(fd, &main_fdset);
						close(fd);
						strcpy(buffer_all, connection_list[fd]);
						strcat(buffer_all, " closed connection");
						
					}
					else
					{
						recv_buffer[r] = '\0';
						
						if(strncmp(recv_buffer, "Shutdown", 8) == 0)
						{
							done = 1;
						}
						strcpy(buffer_all, connection_list[fd]);
						strcat(buffer_all, ">>");
						strcat(buffer_all, recv_buffer);	
					}
					
					for(int i=sockfd+1; i<=MAX_CONNECTION; i++)
					{
						if(i!=fd && FD_ISSET(i, &main_fdset))		//why main_fdset and why not read_fdset?
						{
							printf("Sending Data to FD %d\n", i);
							send(i, buffer_all, strlen(buffer_all), 0);
						}
					}

					if(done)
					{
						close(fd);
						FD_CLR(fd, &main_fdset);
						break;
					}

				}
			}

		}

	}

	close(sockfd);
	for(fd = 1; fd <= MAX_CONNECTION; fd++)
		if(FD_ISSET(fd, &main_fdset))
			close(fd);

	freeaddrinfo(host_server);
	return 0;
}

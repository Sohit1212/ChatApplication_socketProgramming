#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>

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
	int r, sockfd;
	struct addrinfo hints, *host_server;
	short done = 0;
	fd_set read_fdset;
	char recv_buffer[BUFFER_SIZE], send_buffer[BUFFER_SIZE];
	char hostname[BUFFER_SIZE];

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	char *server_ip = "127.0.0.1";
	char *port = "65001";

	r = getaddrinfo(server_ip, port, &hints, &host_server);

	if(r != 0)
	{
		perror("Failed getaddrinfo");
		exit(1);
	}

	sockfd = socket(
		host_server->ai_family,
		host_server->ai_socktype,
		host_server->ai_protocol
		);

	check_result(sockfd, "socket");
	
	r = connect(
		sockfd,
		host_server->ai_addr,
		host_server->ai_addrlen
		);

	check_result(r, "connect");

	if(argc > 1)
	{
		strcpy(hostname,argv[1]);
	}
	else strcpy(hostname,"127.0.0.1");
	
	while(1)
	{	
		FD_ZERO(&read_fdset);
		FD_SET(sockfd, &read_fdset);
		FD_SET(0, &read_fdset);		// add stdin so that fgets does not get blocked
		r = select(sockfd+1, &read_fdset, NULL, NULL, 0); 
		check_result(r, "select");

		/*remote input*/
		if(FD_ISSET(sockfd, &read_fdset))
		{
			r = recv(sockfd, recv_buffer, BUFFER_SIZE, 0);
			if(r < 1)
			{
				perror("Failed, received noting from server");
				break;
			}

			recv_buffer[r] = '\0';

			printf("%s\n",recv_buffer);
		}

		if(FD_ISSET(0, &read_fdset))
		{
			if(fgets(send_buffer, BUFFER_SIZE, stdin) == NULL)
			{
				putchar('\n');
			}
			else if(strncmp(send_buffer, "close", 4) == 0)
			{
				break;
			}
			else
			{
				send(sockfd, send_buffer, strlen(send_buffer), 0);
			}
		}
	}
	
	printf("Disconnected\n");
	close(sockfd);
	freeaddrinfo(host_server);

	return 0;
}

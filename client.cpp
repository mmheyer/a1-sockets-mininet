#include <stdio.h>		// printf()
#include <stdlib.h>		// atoi()
#include <string.h>		// strlen()
#include <sys/socket.h>	// socket(), connect()
#include <unistd.h>		// close()

#include "helpers.h"	// make_client_sockaddr()

static const int MAX_MESSAGE_SIZE = 256;

/**
 * Sends a string message to the server and waits for an integer response.
 *
 * Parameters:
 *		hostname: Remote hostname of the server.
 *		port: Remote port of the server.
 * 		message: The message to send.
 * Returns:
 *		The server's response code on success, -1 on failure.
 */
int send_message(const char *hostname, int port, const char *message) {
	if (strlen(message) > MAX_MESSAGE_SIZE) {
		printf("Error: Message exceeds maximum length\n");
		return -1;
	}

	// TODO: Implement the rest of this function

	// (1) Create a socket
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	// (2) Create a sockaddr_in to specify remote host and port
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
	struct hostent *hp;
	hp = gethostbyname(hostname );
	/*
	 * gethostbyname returns a structure including the network address
	 * of the specified host.
	 */
	if (hp == (struct hostent *) 0) {
			fprintf(stderr, "%s: unknown host\n", hostname);
			exit(2);
	}
	memcpy((char *) &addr.sin_addr, (char *) hp->h_addr, hp->h_length);
	if(make_client_sockaddr(&addr, hostname, port) == -1) {
		return -1;
	}

	// (3) Connect to remote server
	if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("connecting stream socket");
		exit(1);
	}
	// (4) Send message to remote server
	if(send(sock, message, strlen(message), 0) == -1){
		return -1;
	}
	printf("Message sent\n");
	shutdown(sock, SHUT_WR);
	// (5) Wait for integer response
	while(true){
		int loc = 0;
		char buffer[MAX_MESSAGE_SIZE] = {};
		while(true){
				int received = recv(sock, buffer + loc, MAX_MESSAGE_SIZE, 0);
				if(received == -1){
						printf("Something went wrong with receiving...\n");
						return -1;
				}
				printf("Got %d bytes\n", received);
				loc += received;
				if(received == 0){
						break;
				}
		}
		int response = atoi(buffer);
		if(!response){
			return -1;
		}
		else{
			printf("Response: %d\n", response);
			break;
		}
	}
	printf("Request finished...\n");
	// (6) Close connection
	close(sock);
	return 0;
}

int main(int argc, const char **argv) {
	// Parse command line arguments
	if (argc != 4) {
		printf("Usage: ./server hostname port_num message\n");
		return 1;
	}
	const char *hostname = argv[1];
	int port = atoi(argv[2]);
	const char *message = argv[3];
	printf("Sending message %s to %s:%d\n", message, hostname, port);

	int response = send_message(hostname, port, message);
	if (response == -1) {
		printf("Error: Failed to send message\n");
		return 1;
	}
	printf("Server responds with status code %d\n", response);
	return 0;
}

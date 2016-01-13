/*
 * Linux TCP client for cross-partition networking.
 *
 * Copyright (c) 2014-2015 ilbers GmbH
 * Alexander Smirnov <asmirnov@ilbers.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

/* IP address to bind the socket */
#define SOCK_IP_ADDR		"192.167.20.2"
#define SOCK_PORT		3310

#define MAX_MESSAGE_SIZE	128
#define MSG_TYPE_INFO		0x01

typedef struct __attribute__((__packed__)) {
	uint8_t type;
	uint32_t size;
	uint8_t data[MAX_MESSAGE_SIZE];
} app_msg_t;

int main(int argc, char **argv)
{
	int sockfd, portno, n;
	struct sockaddr_in serveraddr;
	app_msg_t msg;

	/* Create the socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		printf("error: socket open failed\n");
		return -1;
	}

	/* Fill the server address */
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_addr.s_addr = inet_addr(SOCK_IP_ADDR);
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(SOCK_PORT);

	/* Create a connection to the server */
	if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
		printf("error: connection failed\n");
		return -2;
	}

	/* Get message from the input */
	printf("Enter a message: ");
	bzero(msg.data, MAX_MESSAGE_SIZE);
	fgets(msg.data, MAX_MESSAGE_SIZE, stdin);

	/* Fill message info */
	msg.size = strlen(msg.data);
	msg.type = MSG_TYPE_INFO;

	/* Send the message to the server */
	n = write(sockfd, (void *)&msg, sizeof(msg));
	if (n < 0) {
		printf("error: writing to socket failed\n");
		return -3;
	}

	printf("Check the FreeRTOS partition, that it has received the message!\n");

	close(sockfd);

	return 0;
}

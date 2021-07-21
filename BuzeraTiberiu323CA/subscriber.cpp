//Copyright[2021] Buzera_Tiberiu 323CA

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"

using namespace std;

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[]) {
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	fd_set readFds;
	fd_set tmpFds;

	int fdMax;

	FD_ZERO(&tmpFds);
	FD_ZERO(&readFds);

	if (argc < 4) {
		usage(argv[0]);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	FD_SET(sockfd, &readFds);
	fdMax = sockfd;
	FD_SET(0, &readFds);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

    ret = send(sockfd, argv[1], BUFLEN, 0);
    DIE(ret < 0, "send");

	while (1) {
		tmpFds = readFds; 

		ret = select(fdMax + 1, &tmpFds, NULL, NULL, NULL);
		DIE(ret < 0, "Nu putem selecta un socket");

		if (FD_ISSET(0, &tmpFds)) {
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);

			// verificam daca comanda este exit
			if (strcmp(buffer, "exit\n") == 0) {
				break;
			}

			// verificam daca comanda este subscribe
			if(strncmp(buffer, "subscribe", 9) == 0) {
				ret = send(sockfd, buffer, BUFLEN, 0);
				DIE(ret < 0, "send_subscribe data");
				cout << "Subscribed to topic." << endl;
			}

			// verificam daca comanda este unsubscribe
			if(strncmp(buffer, "unsubscribe", 11) == 0) {
				ret = send(sockfd, buffer, BUFLEN, 0);
				DIE(ret < 0, "send_unsubscribe data");
				cout << "Unsubscribed from topic." << endl;
			}

			n = send(sockfd, buffer, BUFLEN, 0);
			DIE(n < 0, "SEND FAILED");
		} else if (FD_ISSET(sockfd, &tmpFds)) {
			memset(buffer, 0, BUFLEN);
			int ans = recv(sockfd, buffer, BUFLEN, 0);
			DIE(ans < 0, "recv");
			
			//daca se receptioneaza comanda inchidere se inchide clientul TCP
			//altfel se afisaza mesajul primit de la server
			if(strcmp(buffer, "inchidere") == 0) {
                break;
            } else {
				printf("%s\n", buffer);
			}
		}
	}

	close(sockfd);

	return 0;
}
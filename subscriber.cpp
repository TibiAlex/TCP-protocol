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

bool check_if_winner(int matrix[3][3]) {
	for(int i = 0 ; i < 3; i++) {
		if((matrix[i][0] == 1) && (matrix[i][1] == 1) && (matrix[i][2] == 1)) {
				return true;
		}
		if((matrix[i][0] == 2) && (matrix[i][1] == 2) &&  (matrix[i][2] == 2)) {
				return true;
		}
		 if((matrix[0][i] == 1) &&  (matrix[1][i] == 1) && (matrix[2][2] == 1)) {
			 	return true;
		 }
		 if((matrix[0][i] == 2) &&  (matrix[1][i] == 2) && (matrix[2][2] == 2)) {
			 	return true;
		 }
	}
	if((matrix[0][0] == 1) && (matrix[1][1] == 1) && (matrix[2][2] == 1)) {
		return true;
	}
	if((matrix[0][0] == 2) && (matrix[1][1] == 2) && (matrix[2][2] == 2)) {
		return true;
	}
	if((matrix[0][2] == 1) && (matrix[1][1] == 1) && (matrix[2][0] == 1)) {
		return true;
	}
	if((matrix[0][2] == 2) && (matrix[1][1] == 2) && (matrix[2][0] == 2)) {
		return true;
	}
	return false;
}

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
            } else if(strcmp(buffer, "playx") == 0) {
				cout << "Joci cu X!" << endl;
				ret = send(sockfd, "mutare_X", BUFLEN, 0);
				DIE(ret < 0, "primire matrice x");

			} else if(strcmp(buffer, "playo") == 0) {
				cout << "Joci cu O!" << endl;
				cout << "Asteapta mutarea oponentului tau!" << endl;

			} else if(buffer[0] == '1') {
				//tranformam buferul in matrice
				int matrix[3][3];
				for(int i = 1; i < 10 ; i++) {
					if(buffer[i] == '.') {
						matrix[(i-1)/3][(i-1)%3] = 0;
					} else if(buffer[i] == 'X') {
						matrix[(i-1)/3][(i-1)%3] = 1;
					} else {
						matrix[(i-1)/3][(i-1)%3] = 2;
					}
				}
				//afisam matricea initiala
				for(int i = 0 ; i < 3; i++) {
					for(int j = 0 ; j < 3; j++) {
						cout << matrix[i][j] << " ";
					}
					cout << endl;
				}
				//citim miscari pana se citeste o miscare valida
				int x,y, ok = 0;
				while(ok == 0) {
					cin >> x >> y;
					if(x > 2 || y > 2) {
						cout << "Mutare incorecta!" << endl;
						continue;
					}
					if(matrix[x][y] == 0) {
						matrix[x][y] = 1;
						ok = 1;
					} else {
						cout << "Mutare incorecta!" << endl;
					}
				}
				//afisam matricea noua
				for(int i = 0 ; i < 3; i++) {
					for(int j = 0 ; j < 3; j++) {
						cout << matrix[i][j] << " ";
					}
					cout << endl;
				}

				if(check_if_winner(matrix)) {
					cout << "Felicitari ai castigat!" << endl;
					ret = send(sockfd, "castig_X", BUFLEN, 0);
					DIE(ret < 0, "castig_X");
					continue;
				}
			
				//tranfomam noua matrice in string
				char mat[10];
				int d = 1;
				mat[0] = '2';
				for(int i = 0 ; i < 3; i++) {
					for(int j = 0; j < 3; j++) {
						if(matrix[i][j] == 0) {
							mat[d++] = '.';
						} else if(matrix[i][j] == 1) {
							mat[d++] = 'X';
						} else {
							mat[d++] = 'O';
						}
					}
				}
				//trimitem matricea inapoi la server
				cout << "Se asteapta mutarea oponentului!" << endl;
				ret = send(sockfd, mat, BUFLEN, 0);
				DIE(ret < 0, "primire matrice O");
			} else if(buffer[0] == '2') {
				//tranformam buferul in matrice
				int matrix[3][3];
				for(int i = 1; i < 10 ; i++) {
					if(buffer[i] == '.') {
						matrix[(i-1)/3][(i-1)%3] = 0;
					} else if(buffer[i] == 'X') {
						matrix[(i-1)/3][(i-1)%3] = 1;
					} else {
						matrix[(i-1)/3][(i-1)%3] = 2;
					}
				}
				//afisam matricea initiala
				for(int i = 0 ; i < 3; i++) {
					for(int j = 0 ; j < 3; j++) {
						cout << matrix[i][j] << " ";
					}
					cout << endl;
				}
				//citim miscari pana se citeste o miscare valida
				int x,y, ok = 0;
				while(ok == 0) {
					cin >> x >> y;
					if(x > 2 || y > 2) {
						cout << "Mutare incorecta!" << endl;
						continue;
					}
					if(matrix[x][y] == 0) {
						matrix[x][y] = 2;
						ok = 1;
					} else {
						cout << "Mutare incorecta!" << endl;
					}
				}
				//afisam matricea noua
				for(int i = 0 ; i < 3; i++) {
					for(int j = 0 ; j < 3; j++) {
						cout << matrix[i][j] << " ";
					}
					cout << endl;
				}

				if(check_if_winner(matrix)) {
					cout << "Felicitari ai castigat!" << endl;
					ret = send(sockfd, "castig_O", BUFLEN, 0);
					DIE(ret < 0, "castig_O");
					continue;
				}

				//tranfomam noua matrice in string
				char mat[10];
				int d = 1;
				mat[0] = '1';
				for(int i = 0 ; i < 3; i++) {
					for(int j = 0; j < 3; j++) {
						if(matrix[i][j] == 0) {
							mat[d++] = '.';
						} else if(matrix[i][j] == 1) {
							mat[d++] = 'X';
						} else {
							mat[d++] = 'O';
						}
					}
				}
				//trimitem matricea inapoi la server
				cout << "Se asteapta mutarea oponentului!" << endl;
				ret = send(sockfd, mat, BUFLEN, 0);
				DIE(ret < 0, "primire matrice O");
			} else if(strncmp(buffer, "Ai cam pierdut", 14) == 0) { 
				cout << buffer << endl;
				ret = send(sockfd, "mutare_X", BUFLEN, 0);
				DIE(ret < 0, "primire matrice x");
			}
		}
	}

	close(sockfd);

	return 0;
}
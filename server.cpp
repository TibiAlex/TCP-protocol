//Copyright[2021] Buzera_Tiberiu 323CA

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"

using namespace std;

//structura ce retine informatii despre clienti
struct client{
	string id;
	int socket;
	bool disconnected;
};

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[]) {
	//dezactivam algoritmul lui Neagle
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    int sock_tcp, sock_udp, sock_tmp, portno;

	vector<client> clients_even;
	vector<client> clients_odd;
	vector<vector<vector<int>>> matrix;
	int count = 0;
	
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, sub_addr;
	int n, i, ret;
	socklen_t sublen;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire
	// (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sock_tcp < 0, "socket_tcp");
	sock_udp = socket(PF_INET, SOCK_DGRAM, 0);
	DIE(sock_udp < 0, "socket_udp");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi_portno");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	memset((char *) &sub_addr, 0, sizeof(sub_addr));
	serv_addr.sin_family = sub_addr.sin_family = AF_INET;
	serv_addr.sin_port = sub_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = sub_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sock_tcp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind_tcp");

	ret = bind(sock_udp, (struct sockaddr *) &serv_addr,
		 sizeof(struct sockaddr));
	DIE(ret < 0, "bind_udp");

	ret = listen(sock_tcp, SOMAXCONN);
	DIE(ret < 0, "listen_tcp");

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sock_tcp, &read_fds);
	FD_SET(sock_udp, &read_fds);
	FD_SET(0, &read_fds);

	if (sock_udp > sock_tcp) {
		fdmax = sock_udp;
	} else {
		fdmax = sock_tcp;
	}

	while (1) {
		tmp_fds = read_fds;

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				//verificam daca socketul primit este de la un client TCP
				if (i == sock_tcp) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen), pe care serverul o accepta
					sublen = sizeof(sub_addr);
					sock_tmp = accept(sock_tcp, (struct sockaddr *) &sub_addr, &sublen);
					DIE(sock_tmp < 0, "accept");

					memset(buffer, 0, BUFLEN);
					ret = recv(sock_tmp, buffer, BUFLEN, 0);
					DIE(ret < 0, "recv");

					//cazul in care avem un nou client
					cout << "New client " << buffer << " connected from " << inet_ntoa(sub_addr.sin_addr) << ":" << ntohs(sub_addr.sin_port) << "." << endl;

					client c;
					c.id = string(buffer);
					c.socket = sock_tmp;
					c.disconnected = false;
					count++;
					
					FD_SET(sock_tmp, &read_fds);
					if (sock_tmp > fdmax) { 
						fdmax = sock_tmp;
					}

					if(count%2 != 0) {
						clients_odd.push_back(c);
					} else {
						clients_even.push_back(c);
						vector<vector<int>> m(3, vector<int>(3));
						for(int i = 0; i < 3; i++) {
							for(int j = 0; j < 3; j++) {
								m[i][j] = 0;
							}
						}
						matrix.push_back(m);
						ret = send(clients_odd[clients_odd.size()-1].socket, "playx", BUFLEN, 0);
						DIE(ret < 0, "send_x");
						ret = send(clients_even[clients_odd.size()-1].socket, "playo", BUFLEN, 0);
						DIE(ret < 0, "send_0");
					}

				// citeste de la tastatura comanda exit
				} else if(i == 0) {
					string command;
					cin >> command;
					if(strcmp(command.c_str(), "exit") == 0) {
						for(int j = 3 ; j <= fdmax; j++) {
							if(FD_ISSET(j, &read_fds) && j != sock_udp && j != sock_tcp) {
								ret = send(j, "inchidere", BUFLEN, 0);
								DIE(ret < 0, "send");
								close(j);
							}
						}
						close(sock_udp);
						close(sock_tcp);
						return 0;
					}
				// verificam daca socketul primit este de la un client udp
				} else {
					//s-au primit date pe unul din socketii de client,
					//asa ca serverul trebuie sa le receptioneze
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, BUFLEN, 0);
					DIE(n < 0, "recv");

					// verificam daca s-a deconectat vreun client TCP
					if (n == 0) {
						// conexiunea s-a inchis
						for(unsigned long int j = 0 ; j < clients_odd.size(); j++) {
							if(i == clients_odd[j].socket) {
								printf("Client %s disconnected.\n", clients_odd[j].id.c_str());
								ret = send(clients_even[j].socket, "inchidere", BUFLEN, 0);
								DIE(ret < 0, "send");
								printf("Client %s disconnected.\n", clients_even[j].id.c_str());
								clients_odd.erase(clients_odd.begin() + i);
								clients_even.erase(clients_even.begin() + i);
								break;
							}
						}
						for(unsigned long int j = 0 ; j < clients_even.size(); j++) {
							if(i == clients_even[j].socket) {
								printf("Client %s disconnected.\n", clients_odd[j].id.c_str());
								ret = send(clients_odd[j].socket, "inchidere", BUFLEN, 0);
								DIE(ret < 0, "send");
								printf("Client %s disconnected.\n", clients_even[j].id.c_str());

								clients_odd.erase(clients_odd.begin() + i);
								clients_even.erase(clients_even.begin() + i);
								break;
							}
						}
						// se scoate din multimea de citire socketul inchis 
						close(i);
						FD_CLR(i, &read_fds);
					} else {

						// verificam daca un client a dat comanda subscribe
						if(strncmp(buffer, "mutare_X", 8) == 0) {
							char mat[10];
							int d = 1;
							mat[0] = '1';
							for(int i = 0 ; i < 3; i++) {
								for(int j = 0; j < 3; j++) {
									if(matrix[matrix.size() - 1][i][j] == 0) {
										mat[d++] = '.';
									} else if(matrix[matrix.size() - 1][i][j] == 1) {
										mat[d++] = 'X';
									} else {
										mat[d++] = 'O';
									}
								}
							}
							ret = send(clients_odd[clients_odd.size()-1].socket, mat, BUFLEN, 0);
							DIE(ret < 0, "send_x");
						} else if(buffer[0] == '2') {
							cout << buffer << endl;
							ret = send(clients_even[clients_even.size()-1].socket, buffer, BUFLEN, 0);
							DIE(ret < 0, "send_O");
						} else if(buffer[0] == '1') {
							ret = send(clients_odd[clients_odd.size()-1].socket, buffer, BUFLEN, 0);
							DIE(ret < 0, "send_O");
						} else if(strncmp(buffer, "castig_X", 8) == 0) {
							for(int i = 0 ; i < 3; i++) {
								for(int j = 0; j < 3; j++) {
									matrix[matrix.size() - 1][i][j] = 0;	
								}
							}
							ret = send(clients_even[clients_even.size()-1].socket, "Ai cam pierdut, sorry ;((", BUFLEN, 0);
							DIE(ret < 0, "send_O");
						}	else if(strncmp(buffer, "castig_O", 8) == 0) {
							for(int i = 0 ; i < 3; i++) {
								for(int j = 0; j < 3; j++) {
									matrix[matrix.size() - 1][i][j] = 0;	
								}
							}
							ret = send(clients_odd[clients_odd.size()-1].socket, "Ai cam pierdut, sorry ;((", BUFLEN, 0);
							DIE(ret < 0, "send_X");
						}		
					}
				}
			}
		}
	}

	return 0;
}
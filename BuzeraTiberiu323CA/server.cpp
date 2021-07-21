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
	vector< pair < string, int > > topics;
	vector< string > messages;
	bool disconnected;
	int sf;
};

//structura ce primeste mesajele de la clientul udp
struct udp {
	char topic[50];
	unsigned int tip_date;
	char continut_string[1500];
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
	vector< client > clients;
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

					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					int k = 0;
					for(unsigned long int j = 0 ; j < clients.size(); j++) {
						//cazul in care clientul este deja conectat
						if(strcmp(buffer, clients[j].id.c_str()) == 0 && clients[j].disconnected == false) {
								cout << "Client " << buffer << " already connected." << endl; 
								k = 1;
								ret = send(sock_tmp, "inchidere", BUFLEN, 0);
								DIE(ret < 0, "send exit to already subscriber");
								close(sock_tmp);
								break;
						//cazul in care clientul se reconecteaza
						} else if(strcmp(buffer, clients[j].id.c_str()) == 0 && clients[j].disconnected == true) {
								cout << "New client " << clients[j].id << " connected from " << inet_ntoa(sub_addr.sin_addr) << ":" << ntohs(sub_addr.sin_port) << "." << endl; 
								k = 1;
								clients[j].disconnected = false;
								while(clients[j].messages.size() != 0) {
										ret = send(sock_tmp, clients[j].messages[0].c_str(), BUFLEN, 0);
										DIE(ret < 0, "send old messages");
										clients[j].messages.erase(clients[j].messages.begin());
								}
								clients[j].socket = sock_tmp;
								FD_SET(sock_tmp, &read_fds);
								if (sock_tmp > fdmax) { 
									fdmax = sock_tmp;
								}
								break;
						}
					}

					if(k == 1) {
						continue;
					}
					//cazul in care avem un nou client
					cout << "New client " << buffer << " connected from " << inet_ntoa(sub_addr.sin_addr) << ":" << ntohs(sub_addr.sin_port) << "." << endl;

					client c;
					c.id = string(buffer);
					c.socket = sock_tmp;
					c.disconnected = false;
					clients.push_back(c);
					
					FD_SET(sock_tmp, &read_fds);
					if (sock_tmp > fdmax) { 
						fdmax = sock_tmp;
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
				} else if (i == sock_udp) {
					//structura care retine id-ul si port-ul clientului udp
					sockaddr_in yolo;
					socklen_t skl = sizeof(yolo);

					char mesaj[BUFLEN];
					memset(mesaj, 0, BUFLEN);
					memset(buffer, 0, BUFLEN);
					ret = recvfrom(i, buffer, 1551, 0, (struct sockaddr *)&yolo, &skl);
					DIE(ret < 0, "recv");

					//cream mesajul ce trebuie trimis catre clientul TCP
					strcat(mesaj, inet_ntoa(yolo.sin_addr));
					strcat(mesaj, ":");
					strcat(mesaj, to_string(htons(yolo.sin_port)).c_str());
					udp udp;
					strncpy(udp.topic, buffer, 50); //luam topicul
					udp.tip_date = buffer[50]; //luam tipul de date

					//luam cele 4 tipuri de date si cream stringuri cu fiecare mesaj
					if(udp.tip_date == 3) {
							strncpy(udp.continut_string, buffer + 51, 1500);
					} else if(udp.tip_date == 2) {
							int sign = buffer[51];
							double value = ntohl(*(uint32_t*)(buffer + 52));
							int power = (int)buffer[56];
							double divider = 1;
							for (int i = 0; i < power; i++) {
								divider *= 10;
							}
							float floatNumber = ((float)value) / divider;
							if (sign == 1) {
								floatNumber *= -1;
							}
							strcpy(udp.continut_string ,to_string(floatNumber).c_str());
					} else if(udp.tip_date == 1) {
							double value = 1.0 * ntohs(*(uint16_t*)(buffer + 51));
							sprintf(udp.continut_string, "%.2lf", value/100);
					} else if(udp.tip_date == 0) {
							int sign = buffer[51];
							long value = ntohl(*(uint32_t*)(buffer + 52));
							if(sign == 1) {
								value *= -1;
							}
							strcpy(udp.continut_string ,to_string(value).c_str());
					}
					
					strcat(mesaj, " - ");
					strcat(mesaj, udp.topic);
					strcat(mesaj, " - ");
					if(udp.tip_date == 0) {
						strcat(mesaj, "INT");
					} else if(udp.tip_date == 1) {
						strcat(mesaj, "SHORT_REAL");
					} else if(udp.tip_date == 2) {
						strcat(mesaj, "FLOAT");
					} else {
						strcat(mesaj, "STRING");
					}
					strcat(mesaj, " - ");
					strcat(mesaj, udp.continut_string);

					// cautam clientii cu topic si trimitem mesajul sau il punem in coada
					for(unsigned long int j = 0; j < clients.size(); j++) {
						for(unsigned long int k = 0 ; k < clients[j].topics.size(); k++) {
							if(strcmp(clients[j].topics[k].first.c_str(), udp.topic) == 0 && clients[j].topics[k].second == 1 && clients[j].disconnected == true) {
								clients[j].messages.push_back(mesaj);
							} else if(strcmp(clients[j].topics[k].first.c_str(), udp.topic) == 0 && clients[j].disconnected == false) {
								ret = send(clients[j].socket, mesaj, BUFLEN, 0);
								DIE(ret < 0, "send_mesaj");
							}
						}
					}
				// primim comenzi de la clientii TCP
				} else {
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, BUFLEN, 0);
					DIE(n < 0, "recv");

					// verificam daca s-a deconectat vreun client TCP
					if (n == 0) {
						// conexiunea s-a inchis
						for(unsigned long int j = 0 ; j < clients.size(); j++) {
							if(i == clients[j].socket) {
								printf("Client %s disconnected.\n", clients[j].id.c_str());
								clients[j].disconnected = true;
								clients[j].socket = -1;
								break;
							}
						}

						// se scoate din multimea de citire socketul inchis 
						close(i);
						FD_CLR(i, &read_fds);
					} else {
						// verificam daca un client a dat comanda subscribe
						if(strncmp(buffer, "subscribe", 9) == 0) {
							char topic[50], subs[20];
							int sf;
							sscanf(buffer, "%s %s %d", subs, topic, &sf);
							for(unsigned long int j = 0 ; j < clients.size(); j++) {
								if(i == clients[j].socket) {
									int ok = 0;
									for(unsigned long int index = 0; index < clients[j].topics.size(); index++) {
											if(strcmp(clients[j].topics[index].first.c_str(), topic) == 0) {
													clients[j].topics[index].second = sf;
													ok = 1;
													break;
											}
									}
									if(ok == 0) {
										clients[j].topics.push_back(make_pair(string(topic), sf));
									}
								}
							}
						}
						// verificam daca un client a dat comanda unsubscribe
						if(strncmp(buffer, "unsubscribe", 11) == 0) {
							char topic[50], subs[20];
							sscanf(buffer, "%s %s", subs, topic);
							for(unsigned long int j = 0 ; j < clients.size(); j++) {
								if(i == clients[j].socket) {
									for(unsigned long int index = 0; index < clients[j].topics.size(); index++) {
										if(strcmp(clients[j].topics[index].first.c_str(), topic) == 0) {
											clients[j].topics.erase(clients[j].topics.begin() + index);
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return 0;
}
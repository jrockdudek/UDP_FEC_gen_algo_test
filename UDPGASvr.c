#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include "data.h"

int main(int argc, char* argv[]) {
	int server_sockfdUDP, server_sockfdTCP, client_sockfdTCP;
	int server_lenUDP, server_lenTCP, client_lenTCP, client_lenUDP;
	int port;
	struct sockaddr_in server_addressUDP, server_addressTCP;
	struct sockaddr_in client_addressTCP, client_addressUDP;
	fd_set my_fds;
	struct timeval myTimeOut;
	char buf[20000];
	char* outMsg;
	int bytesRecv, ret;
	int i;
	int sizeOfPackets, numOfPacksRecv, packetNum;
	char packetsRecv[4000]; //0 if a certain packet hasn't been received, 1 if it has been
	
	if(argc != 2) {
		printf("Requires one argument, the port number.\n");
		return 0;
	}
	
	port = atoi(argv[1]);

	
	server_sockfdUDP = socket(AF_INET, SOCK_DGRAM, 0);
	if(server_sockfdUDP == -1) {
		perror(strerror(errno));
		return 0;
	}
	
	server_sockfdTCP = socket(AF_INET, SOCK_STREAM, 0);
	if(server_sockfdTCP == -1) {
		perror(strerror(errno));
		return 0;
	}	
	
	
	
	server_addressUDP.sin_family = AF_INET;
	server_addressUDP.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addressUDP.sin_port = htons(port);
	server_lenUDP = sizeof(server_addressUDP);
	if(bind(server_sockfdUDP, (struct sockaddr *)&server_addressUDP, server_lenUDP) == -1) {
		perror(strerror(errno));
		return 0;
	}
	
	server_addressTCP.sin_family = AF_INET;
	server_addressTCP.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addressTCP.sin_port = htons(port);
	server_lenTCP = sizeof(server_addressTCP);
	if(bind(server_sockfdTCP, (struct sockaddr *)&server_addressTCP, server_lenTCP) == -1) {
		perror(strerror(errno));
		return 0;
	}
	
	if(listen(server_sockfdTCP, 5) == -1) {
		perror(strerror(errno));
		return 0;
	}
	
	//First the server waits for a TCP connection. Once that is made, the size of packets
	//that are going to be sent is received using the TCP connection. Then the server waits for UDP packets.
	//After two seconds of not receiving a packet, sends the number of packets lost back
	//using the TCP connection
	
	
	while(1) {

		
		client_lenTCP = sizeof(client_addressTCP);
		client_sockfdTCP = accept(server_sockfdTCP, (struct sockaddr*)&client_addressTCP, &client_lenTCP);
		
		if(client_sockfdTCP == -1) {
			perror(strerror(errno));
			return 0;
		}
		
		//Getting the packet size
		read(client_sockfdTCP, buf, sizeof(buf));
		sizeOfPackets = atoi(buf);
		printf("size: %i\n", sizeOfPackets);
		for(i = 0; i < 4000; i++) {
			packetsRecv[i] = 0;
		}
		numOfPacksRecv = 0;
		//Now ready to receive packets
		while(1) {
			myTimeOut.tv_sec = 2;
			myTimeOut.tv_usec = 0;
			FD_ZERO(&my_fds);
			
			FD_SET(server_sockfdUDP, &my_fds);
			myTimeOut.tv_sec = 10;
			myTimeOut.tv_usec = 0;
			ret = select(server_sockfdUDP + 1, &my_fds, NULL, NULL, &myTimeOut);
			if(ret == -1)//select fail
			{
				perror("on select");
				return 0; 
			}
			else if(ret == 0) //timeout
			{
				break;
			}
			else
			{
				bytesRecv = recvfrom(server_sockfdUDP, buf, sizeof(buf), 0, NULL, NULL);
				//printf("bytes: %i", bytesRecv);
				//printf("atoi(buf): %i\n", atoi(buf));
				if(packetsRecv[atoi(buf)] == 0) {
					packetsRecv[atoi(buf)] = 1;
					numOfPacksRecv++;
				}
			}
		}
		printf("packetsRecv: %i\n", numOfPacksRecv);
		sprintf(buf, "%i", numOfPacksRecv);
		write(client_sockfdTCP, buf, sizeof(buf));
		close(client_sockfdTCP);

	}
}


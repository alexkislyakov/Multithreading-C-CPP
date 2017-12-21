#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

//sudo dtruss 
//telnet 127.0.0.1 12345

const int BUFFER_SIZE = 1024;

void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[argc])
{
    int MasterSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (MasterSocket < 0) {
        error("Error opening socket");
    }

    int optval = 1;
    setsockopt(MasterSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in SockAddr;
    SockAddr.sin_family = AF_INET; // IPv4
    SockAddr.sin_port = htons(12345); //Port 12345
    SockAddr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0
    
    if (bind(MasterSocket, (struct sockaddr*)(&SockAddr), sizeof(SockAddr)) < 0) {
        error("Error on binding");
    }

    if (listen(MasterSocket, SOMAXCONN) < 0) {
        error("Error on listen");
    }

    while(1) {
	int SlaveSocket = accept(MasterSocket, 0, 0);

	if (SlaveSocket < 0) {
	    error("Error on accept");
	}
	
	char Buffer[BUFFER_SIZE];

	int read = recv(SlaveSocket, Buffer, BUFFER_SIZE, 0);
        if (read < 0) {
	   error("Error reading from socket");
  	}
	if (!read) break;

	int nsend = send(SlaveSocket, Buffer, read, 0);
	if (nsend < 0) {
            error("Error writing to socket");
	}
	
	shutdown(SlaveSocket, SHUT_RDWR);
	close(SlaveSocket);
    }
    
    return 0;
}

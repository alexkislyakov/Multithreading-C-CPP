#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

const int BUFFER_SIZE = 1024;

void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[argc])
{
    int Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (Socket < 0) {
	error("Error opening socket");
    }

    int optval = 1;
    setsockopt(Socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in SockAddr;
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = htons(12345);
    SockAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (connect(Socket, (struct sockaddr *)(&SockAddr), sizeof(SockAddr)) < 0) {
        error("Error connecting");
    }

    char Buffer[BUFFER_SIZE];
    char txt[] = "Screw you guys, i'm going home";

    int send_bytes = send(Socket, txt, sizeof(txt), 0);
    if (send_bytes < 0) {
	error("Error writing to socket");
    }
    int get_bytes = recv(Socket, Buffer, sizeof(Buffer), 0);
    if (get_bytes < 0) {
	error("Error reading from socket");
    }

    printf("%s\n", Buffer);

    shutdown(Socket, SHUT_RDWR);
    close(Socket);

    return 0;
}

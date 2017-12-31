#include <unordered_map>
#include <sstream>

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/event.h>
#include <arpa/inet.h>

const int BUFFER_SIZE = 1024;
const int MAX_EVENTS = 32;

void error(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

int set_nonblock(int fd) {
    int flags;
#if defined(O_NONBLOCK)
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
	flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(fd, FIOBIO, &flags);
#endif
}

void send_msg(int descriptor, const char *message, std::unordered_map<unsigned, sockaddr_in> &clients) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clients[descriptor].sin_addr, ip, INET_ADDRSTRLEN);
    std::stringstream msg;
    msg << ip << ": " << message;

    for (auto client : clients) {
	int SendSize = send(client.first, msg.str().c_str(), msg.str().length(), 0);
        if (SendSize < 0) {
            perror("Error writing to socket");
        }
    }
}

int main(int argc, char *argv[argc])
{
    int MasterSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (MasterSocket < 0) {
        error("Error opening socket");
    }

    int optval = 1;
    setsockopt(MasterSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    std::unordered_map<unsigned, sockaddr_in> SlaveSockets;

    struct sockaddr_in SockAddr;
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = htons(12345);
    SockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(MasterSocket, (struct sockaddr*)(&SockAddr), sizeof(SockAddr)) < 0) {
        error("Error on binding");
    }

    set_nonblock(MasterSocket);

    if (listen(MasterSocket, SOMAXCONN) < 0) {
        error("Error on listen");
    }

    int kq;
    if ((kq = kqueue()) == -1) {
	error("kqueue");
    }

    struct kevent ke;
    struct kevent el[MAX_EVENTS];
    bzero(&ke, sizeof(ke));
    bzero(&el, sizeof(el));

    EV_SET(&ke, MasterSocket, EVFILT_READ, EV_ADD, 0, 0, NULL);

    if (kevent(kq, &ke, 1, NULL, 0, NULL) == -1) {
        error("kevent");
    }

    while (1) {
	bzero(&el, sizeof(el));
	int N = kevent(kq, NULL, 0, el, MAX_EVENTS, NULL);

	for (unsigned int i = 0; i < N; ++i) {
	    if (el[i].flags & EV_EOF) {
		const char msg[] = "Disconnected\n";
	        send_msg(el[i].ident, msg, SlaveSockets);
		shutdown(el[i].ident, SHUT_RDWR);
		close(el[i].ident);
		SlaveSockets.erase(el[i].ident);
	    } else if (el[i].filter == EVFILT_READ) {
	        if (el[i].ident == MasterSocket) {
		    struct sockaddr_in ClientAddr;
		    socklen_t client_addr_size = sizeof(ClientAddr); 
		    int SlaveSocket = accept(MasterSocket, (struct sockaddr*)(&ClientAddr), &client_addr_size);
		    if (SlaveSocket == -1) {
		        perror("Error on accept");
		    }
	            set_nonblock(SlaveSocket);
		    bzero(&ke, sizeof(ke));
		    EV_SET(&ke, SlaveSocket, EVFILT_READ, EV_ADD, 0, 0, 0);
		    if (kevent(kq, &ke, 1, NULL, 0, NULL) == -1) {
                        perror("kevent");
                    }
		    SlaveSockets[SlaveSocket] = ClientAddr;
                    const char msg[] = "Connected\n";
		    send_msg(SlaveSocket, msg, SlaveSockets);
	        } else {
		    static char Buffer[BUFFER_SIZE];
		    memset(Buffer, 0, BUFFER_SIZE);
		    int RecvSize = recv(el[i].ident, Buffer, BUFFER_SIZE, 0);
		    if (RecvSize > 0) {
                        send_msg(el[i].ident, Buffer, SlaveSockets);
		    }
	        }
	    }
	}
    }
    
    return 0;
}

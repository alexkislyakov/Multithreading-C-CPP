#include <iostream>
#include <set>
#include <algorithm>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

const int POLL_SIZE = 2048;
const int BUFFER_SIZE = 1024;

void error(const std::string &msg) {
    perror(msg.c_str());
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

int main(int argc, char *argv[argc])
{
    int MasterSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (MasterSocket < 0) {
        error("Error opening socket");
    }

    int optval = 1;
    setsockopt(MasterSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    
    std::set<int> SlaveSockets;

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

    struct pollfd Set[POLL_SIZE];
    Set[0].fd = MasterSocket;
    Set[0].events = POLLIN;

    while (true) {
        unsigned int Index = 1;
        for (auto Iter = SlaveSockets.begin(); Iter != SlaveSockets.end(); Iter++) {
            Set[Index].fd = *Iter;
            Set[Index].events = POLLIN;
            Index++;
        }
        unsigned int SetSize = 1 + SlaveSockets.size();

        poll(Set, SetSize, -1);

        for (unsigned int i = 0; i < SetSize; ++i) {
            if (Set[i].revents & POLLIN) {
                if (i) {
                    static char Buffer[BUFFER_SIZE];
                    int RecvSize = recv(Set[i].fd, Buffer, BUFFER_SIZE, 0);
                    if ((RecvSize == 0) && (errno != EAGAIN)) {
                        shutdown(Set[i].fd, SHUT_RDWR);
                        close(Set[i].fd);
                        SlaveSockets.erase(Set[i].fd);
                    } else if (RecvSize > 0) {
                        int SendSize = send(Set[i].fd, Buffer, RecvSize, 0);
                        if (SendSize < 0) {
                            perror("Error writing to socket");
                        }
                    } else {
                        perror("Error reading from socket");
                    }
                } else {
                    int SlaveSocket = accept(MasterSocket, 0, 0);
                    if (SlaveSocket != -1) {
                        set_nonblock(SlaveSocket);
                        SlaveSockets.insert(SlaveSocket);
                    } else {
                        perror("Error on accept");
                    }
                }
            }
        }
    }
    
    return EXIT_SUCCESS;
}

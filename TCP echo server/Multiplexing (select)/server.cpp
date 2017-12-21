#include <iostream>
#include <set>
#include <algorithm>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

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

    while (true) {
        fd_set Set;
        FD_ZERO(&Set);
        FD_SET(MasterSocket, &Set);
        for (auto Iter = SlaveSockets.begin(); Iter != SlaveSockets.end(); ++Iter) {
            FD_SET(*Iter, &Set);
        }

        int s = 0;
        if (!SlaveSockets.empty()) {
            s = *SlaveSockets.rbegin();
        }
        int Max = std::max(MasterSocket, s);

        select(Max+1, &Set, NULL, NULL, NULL);

        for (auto Iter = SlaveSockets.begin(); Iter != SlaveSockets.end();) {
            if (FD_ISSET(*Iter, &Set)) {
                static char Buffer[BUFFER_SIZE];
                int RecvSize = recv(*Iter, Buffer, BUFFER_SIZE, 0);
                if ((RecvSize == 0) && (errno != EAGAIN)) {
                    shutdown(*Iter, SHUT_RDWR);
                    close(*Iter);
                    Iter = SlaveSockets.erase(Iter);
                    continue;
                } else if (RecvSize > 0) {
                    int SendSize = send(*Iter, Buffer, RecvSize, 0);
                    if (SendSize < 0) {
                        perror("Error writing to socket");
                    }
                } else {
                    perror("Error reading from socket");
                }
            }
            Iter++;
        }
	
        if (FD_ISSET(MasterSocket, &Set)) {
            int SlaveSocket = accept(MasterSocket, 0, 0);
            if (SlaveSocket != -1) {
                set_nonblock(SlaveSocket);
                SlaveSockets.insert(SlaveSocket);
            } else {
                perror("Error on accept");
            }
        }
    }
    
    return EXIT_SUCCESS;
}

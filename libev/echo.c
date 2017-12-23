#include <stdio.h>
#include <netinet/in.h>
#include <ev.h>
#include <stdlib.h>

void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
    char buffer[1024];
    size_t r = recv(watcher->fd, buffer, 1024, 0);
    if (r < 0) {
	return;
    } else if (r == 0) {
	ev_io_stop(loop, watcher);
	free(watcher);
	return;
    } else {
	send(watcher->fd, buffer, r, 0);
    }
}

void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
    int client_sd = accept(watcher->fd, 0, 0);

    struct ev_io *w_client = (struct ev_io*)malloc(sizeof(struct ev_io));
    ev_io_init(w_client, read_cb, client_sd, EV_READ);
    ev_io_start(loop, w_client);
}

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[argc]) {
    struct ev_loop *loop = ev_default_loop(0);

    int sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sd < 0) {
        error("Error opening socket");
    }
    
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
	error("Error on binding");
    }

    if (listen(sd, SOMAXCONN) < 0) {
	error("Error on listen");
    }

    struct ev_io w_accept;
    ev_io_init(&w_accept, accept_cb, sd, EV_READ);
    ev_io_start(loop, &w_accept);

    while(1) ev_loop(loop, 0);

    return 0;
}


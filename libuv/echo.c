#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

uv_tcp_t server;
uv_loop_t *loop;

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void read_cb(uv_stream_t *stream, size_t nread, uv_buf_t *buf) {
    uv_write_t *req = (uv_write_t*)malloc(sizeof(uv_write_t));
    if (nread < 0) {
	if ((nread & UV_EOF) != 0) {
	    perror("Error reading");
	}
	uv_close((uv_handle_t*)stream, NULL);
    }

    uv_buf_t buf_write;
    buf_write.base = (char*)malloc(nread);
    buf_write.len = nread;
    memcpy(buf_write.base, buf->base, nread);
    int r = uv_write(req, stream, &buf_write, 1, NULL);
    free(buf_write.base);

    if (r < 0) {
	perror("Error on writing");
    }
    free(buf->base);
}

void alloc_buffer(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
    buf->base = (char*)malloc(size);
    buf->len = size;
}

void connection_cb(uv_stream_t *server, int status) {
    uv_tcp_t *client = malloc(sizeof(uv_tcp_t));

    if (status < 0) {
	perror("Error on listen");
    }

    uv_tcp_init(loop, client);
    if (uv_accept(server, (uv_stream_t*) client) == 0) {
	int r = uv_read_start((uv_stream_t*)client, alloc_buffer, read_cb);
	if (r < 0) {
	    perror("Error reading");
	}
    } else {
	uv_close((uv_handle_t*)client, NULL);
    }
}

int main(int argc, char *argv[argc])
{
    loop = uv_default_loop();
    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", 12345, &addr);

    uv_tcp_init(loop, &server);
    uv_tcp_bind(&server,(const struct sockaddr*)&addr, 0);
    int r = uv_listen((uv_stream_t*)&server, 128, connection_cb);
    if (r < 0) {
	error("Error on listen");
    }
    
    return uv_run(loop, UV_RUN_DEFAULT);
}

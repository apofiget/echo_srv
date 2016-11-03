/*!
  Description:

  ECHO server based on libev.
  Accept new connection, receive data and send it back
  to connected client.

  How to run:

  make & ./echo_srv [port_number]
  , where port_number port which server
  will listen for a new connections. Default port number - 1025.
  For a stop execution, just press: <Ctrl+C> keys combination.

  Required libev for compilation and work.

  Author: Andrey Andruschenko <apofiget@gmail.com>

  License: Public domain
*/
#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include <err.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "echo_srv.h"

/*!
  Bind and listen for new connections on created socket
  \param listen_addr - pointer to sockaddr_in
  \param mode - 1 for non-blocked operation, 0 otherwise
  \return new FD or -1 in case of error
*/
int listen_on_socket(struct sockaddr_in *listen_addr, int mode) {
    int listen_fd;
    int on = 1;

    if (mode)
        listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    else
        listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (listen_fd < 0) return -1;

    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    setsockopt(listen_fd, IPPROTO_TCP, TCP_QUICKACK, &on, sizeof(on));
    setsockopt(listen_fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));

    if (bind(listen_fd, (const struct sockaddr *)listen_addr, sizeof(struct sockaddr_in)) < 0)
        return -1;

    if (listen(listen_fd, BACKLOG_SIZE)) return -1;

    return listen_fd;
}

/*!
  Read data to buffer from given FD
  \param fd - source FD
  \param pkt - pointer to destination buffer_t
  \return 1 on success, -1 on case of error, 0 in case we should try to read data again later
*/
int read_data(int fd, buffer_t *pkt) {
    int rc = -1;
    volatile int rcv;

    if (pkt == NULL || pkt->buffer == NULL || fd <= 0) return rc;

    while (pkt->offset < (int)pkt->buf_len) {
        rcv = recv(fd, pkt->buffer + pkt->offset, pkt->buf_len - pkt->offset, 0);

        if (rcv < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                rc = 0;
            else
                rc = -1;

            break;
        }

        if (rcv == 0) {
            rc = -1;
            break;
        }

        pkt->rcv += rcv;
        pkt->offset += rcv;
        rc = 1;
    }

    return rc;
}

/*!
  Send data from buffer to given FD
  \param fd - destination FD
  \param pkt - pointer to source buffer_t
  \return 1 on success, -1 on case of error, 0 in case we still have data to send
*/
int send_data(int fd, buffer_t *pkt) {
    int rc = -1;
    volatile int snt = 0;

    if (pkt == NULL || pkt->buffer == NULL || fd <= 0) return rc;

    while (pkt->rcv) {
        snt = send(fd, pkt->buffer + (pkt->offset - pkt->rcv), pkt->rcv, 0);

        if (snt <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                rc = 0;
            else
                rc = -1;

            break;
        }

        pkt->rcv -= snt;
    }

    if (pkt->rcv == 0) {
        pkt->offset = 0;
        rc = 1;
    }

    return rc;
}

/*!
  Callback executed when data from socket maybe read
  \param loop - pointer to ev_loop(main loop structrure)
  \param w - pointer to ev_io(watcher for current FD)
  \param revents - eventmask
*/
void on_read(struct ev_loop *loop, ev_io *w, __attribute__((unused)) int revents) {
    int rc, fd = w->fd;
    buffer_t *buf = (buffer_t *)w->data;

    rc = read_data(fd, buf);

    if (rc >= 0) {
        fprintf(stderr, "[%d] RECEIVED: %.*s\n", fd, buf->rcv, buf->buffer);
        rc = send_data(fd, buf);
    }

    if (rc < 0) {
        fprintf(stderr, "[%d] Connection closed!\n", fd);
        ev_io_stop(loop, w);
        close(fd);
        free(w);
        free(buf->buffer);
        free(buf);
    }
}

/*!
  Callback executed when new connection maybe accepted
  \param loop - pointer to ev_loop(main loop structrure)
  \param w - pointer to ev_io(watcher for current FD)
  \param revents - eventmask
*/
void accept_connect(struct ev_loop *loop, ev_io *w, __attribute__((unused)) int revents) {
    struct ev_io *io = NULL;
    struct sockaddr_in sa;
    socklen_t sa_len = sizeof(sa);
    int fd;
    buffer_t *buf = NULL;

    if ((io = calloc(1, sizeof(struct ev_io))) == NULL) err(EXIT_FAILURE, "calloc() ");

    if ((fd = accept(w->fd, (struct sockaddr *)&sa, &sa_len)) <= 0) return;

    fcntl(fd, F_SETFL, O_NONBLOCK);

    printf("[%d] Accept new connection from %s:%d\n", fd, inet_ntoa(sa.sin_addr),
           ntohs(sa.sin_port));

    if ((buf = calloc(1, sizeof(buffer_t))) == NULL) err(EXIT_FAILURE, "calloc()");
    if ((buf->buffer = calloc(BUFFER_SZ, sizeof(char))) == NULL) err(EXIT_FAILURE, "calloc()");
    buf->buf_len = BUFFER_SZ;

    ev_io_init(io, on_read, fd, EV_READ);
    io->data = buf;
    ev_io_start(loop, io);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in sock = {0};
    struct ev_loop *loop = NULL;
    ev_io sock_watcher;
    int s_fd;
    uint16_t port;

    if (argc > 1)
        port = atoi(argv[1]);
    else
        port = PORT_NUMBER;

    sock.sin_port = htons(port);
    sock.sin_addr.s_addr = htonl(INADDR_ANY);
    sock.sin_family = AF_INET;

    if ((s_fd = listen_on_socket(&sock, 1)) == -1)
        err(EXIT_FAILURE, "listen_on_socket() ");
    else
        fprintf(stderr, "[%d] ECHO server listen on port %d\n", s_fd, port);

    if ((loop = ev_loop_new(EVFLAG_NOENV | EVBACKEND_EPOLL)) == NULL) {
        close(s_fd);
        err(EXIT_FAILURE, "Couldn't create main eventloop! ");
    }

    ev_io_init(&sock_watcher, accept_connect, s_fd, EV_READ);
    ev_io_start(loop, &sock_watcher);

    ev_run(loop, 0);

    exit(EXIT_SUCCESS);
}

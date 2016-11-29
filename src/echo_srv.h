
#ifndef __ECHO_SRV_H_
#define __ECHO_SRV_H_

#define PORT_NUMBER 1025
#define BACKLOG_SIZE 64
#define BUFFER_SZ 256

/*!
  Packet buffer
*/
typedef struct __pkt_t_ {
    char *buffer;         ///< Buffer memory
    volatile int rcv;     ///< Current length of data in buffer
    volatile int offset;  ///< Start writing from this offset
    size_t buf_len;       ///< Buffer total length
} buffer_t;

void release_connect(int fd, struct ev_loop *loop, ev_io *io);
void accept_connect(struct ev_loop *loop, ev_io *w, int revents);
void on_read(struct ev_loop *loop, ev_io *w, int revents);
int send_data(int fd, buffer_t *pkt);
int read_data(int fd, buffer_t *pkt);
int listen_on_socket(struct sockaddr_in *listen_addr, int mode);

#endif

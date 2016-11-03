
#ifndef __ECHO_SRV_H_
#define __ECHO_SRV_H_

#define PORT_NUMBER 1025
#define BACKLOG_SIZE 64
#define BUFFER_SZ 4096

/*!
  Packet buffer
*/
typedef struct __pkt_t_ {
    char *buffer;         ///< Buffer memory
    volatile int rcv;     ///< Current length of data in buffer
    volatile int offset;  ///< Start writing from this offset
    size_t buf_len;       ///< Buffer total length
} buffer_t;

#endif

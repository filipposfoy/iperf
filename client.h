#ifndef CLIENT_H
#define CLIENT_H

#include <stdint.h>

void start_tcp_client(char *server_address, int port, void *config);

void udp_sender(const char *dest_ip, int port, int packet_size, 
    uint64_t bandwidth_bps, double duration_sec);

#endif
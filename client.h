#ifndef CLIENT_H
#define CLIENT_H

#include <stdint.h>

#define MAX_MEASUREMENTS 10000


void start_tcp_client(char *server_address, int port, void *config);

int compare_doubles(const void *a, const void *b);

void udp_client_duration(const char *server_ip, int port, double duration_sec);


void udp_sender(const char *dest_ip, int port, int packet_size, 
    uint64_t bandwidth_bps, double duration_sec);

#endif
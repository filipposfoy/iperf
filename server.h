#ifndef SERVER_H
#define    SERVER_H

#include <stdint.h>
#include "requirements.h"

void start_tcp_server(int port, Config *received_config);

void *handle_client(void *arg);

// void udp_receiver(int port, int payload_size, uint64_t bytes_to_be_recvd);
void udp_receiver(int port, int payload_size, double duration_sec);

uint64_t calculate_total_payload_bytes(int packet_size, 
                                       uint64_t bandwidth_bps, double duration_sec);


#endif
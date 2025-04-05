#ifndef SERVER_H
#define    SERVER_H

#include <stdint.h>
#include "requirements.h"


typedef struct {
    double timestamp;
    uint64_t total_payload;
    uint64_t total_transmitted;
    double goodput_mbps;
    double throughput_mbps;
    double avg_jitter_us;
} PerSecondStats;

void start_tcp_server(int port, Config *received_config);

void *handle_client(void *arg);

// void udp_receiver(int port, int payload_size, uint64_t bytes_to_be_recvd);
void udp_receiver(int port, int payload_size, double duration_sec);

uint64_t calculate_total_payload_bytes(int packet_size, 
                                       uint64_t bandwidth_bps, double duration_sec);


#endif
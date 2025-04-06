#include "client.h"
#include "server.h"
#include "requirements.h"

void print_config(Config *config) {
    printf("Mode: %s\n", config->is_server ? "Server" : (config->is_client ? "Client" : "Unknown"));
    if (config->address) printf("Address: %s\n", config->address);
    if (config->port) printf("Port: %d\n", config->port);
    if (config->interval) printf("Interval: %d sec\n", config->interval);
    if (config->filename) printf("Output File: %s\n", config->filename);
    if (config->udp_packet_size) printf("UDP Packet Size: %d bytes\n", config->udp_packet_size);
    if (config->bandwidth) printf("Bandwidth: %d bps\n", config->bandwidth);
    if (config->num_streams) printf("Parallel Streams: %d\n", config->num_streams);
    if (config->duration) printf("Duration: %d sec\n", config->duration);
    if (config->measure_delay) printf("Measuring One-way Delay\n");
    if (config->wait_time) printf("Wait Time Before Start: %d sec\n", config->wait_time);
}



int main(int argc, char *argv[]) {
    Config config = {0};
    Config* recvd_conf; 
    int opt;
    int client_s; 

    while ((opt = getopt(argc, argv, "sca:p:i:f:l:b:n:t:dw:")) != -1) {
        switch (opt) {
            case 's':
                config.is_server = 1;
                break;
            case 'c':
                config.is_client = 1;
                break;
            case 'a':
                config.address = optarg;
                break;
            case 'p':
                config.port = atoi(optarg);
                break;
            case 'i':
                config.interval = atoi(optarg);
                break;
            case 'f':
                config.filename = optarg;
                break;
            case 'l':
                config.udp_packet_size = atoi(optarg);
                break;
            case 'b':
                config.bandwidth = atoi(optarg);
                break;
            case 'n':
                config.num_streams = atoi(optarg);
                break;
            case 't':
                config.duration = atoi(optarg);
                break;
            case 'd':
                config.measure_delay = 1;
                break;
            case 'w':
                config.wait_time = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s -s|-c [-a address] [-p port] ...\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (!config.is_server && !config.is_client) {
        fprintf(stderr, "Error: You must specify either server (-s) or client (-c) mode.\n");
        exit(EXIT_FAILURE);
    }

    print_config(&config);

    if (config.is_server) {
        Config *recvd_conf;
        recvd_conf = start_tcp_server(config.port ? config.port : PORT, recvd_conf);
        printf("Packet size :: %d\n", recvd_conf->udp_packet_size);
        printf("Na metrhsw to 1 way delay : %d\n\n\n", recvd_conf->measure_delay);
        // print_config(recvd_conf);
        // printf("Bandwidth :: %d\n", config.bandwidth);
        if(recvd_conf->measure_delay){
            udp_server(config.port ? config.port : PORT_UDP);
        }else{
            udp_receiver(config.port ? config.port : PORT_UDP, recvd_conf->udp_packet_size ? recvd_conf->udp_packet_size : 1024, recvd_conf->duration ? recvd_conf->duration : 10);
        }
        }else if (config.is_client) {
        if (!config.address) {
            fprintf(stderr, "Error: Client mode requires server address (-a).\n");
            exit(EXIT_FAILURE);
        }
        print_config(&config);
        if(config.wait_time != 0){
            sleep(config.wait_time);
        }
        start_tcp_client(config.address, config.port ? config.port : PORT,&config);
        if(config.measure_delay){
            udp_client_duration(config.address, config.port ? config.port : PORT_UDP, config.duration ? config.duration : 10);
        }else{        
            udp_sender(config.address,config.port ? config.port : PORT_UDP, config.udp_packet_size ? config.udp_packet_size : 1024, config.bandwidth ? config.bandwidth : 1000000, 
            config.duration ? config.duration : 10);
        }
    }

    return 0;
}

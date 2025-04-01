#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <math.h>

#define HEADER_SIZE 8  // Fixed header size
#define MAX_CLIENTS 10 // Max concurrent clients
#define PORT 8080

#define ETHERNET_HEADER_SIZE 14   // Ethernet header (without VLAN)
#define IP_HEADER_SIZE 20         // IPv4 header (without options)
#define UDP_HEADER_SIZE 8         // UDP header
#define TOTAL_HEADER_SIZE (ETHERNET_HEADER_SIZE + IP_HEADER_SIZE + UDP_HEADER_SIZE)

#define DEFAULT_PACKET_SIZE 1024
#define DEFAULT_NUM_PACKETS 1000
#define DEFAULT_PORT 5000


typedef struct {
    int is_server;
    int is_client;
    char *address;
    int port;
    int interval;
    char *filename;
    int udp_packet_size;
    int bandwidth;
    int num_streams;
    int duration;
    int measure_delay;
    int wait_time;
} Config;

typedef struct {
    uint16_t msg_type;  
    uint16_t msg_length; 
    uint32_t timestamp;  
} Header;
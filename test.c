#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>
#include <math.h>


#define ETHERNET_HEADER_SIZE 14   // Ethernet header (without VLAN)
#define IP_HEADER_SIZE 20         // IPv4 header (without options)
#define UDP_HEADER_SIZE 8         // UDP header
#define TOTAL_HEADER_SIZE (ETHERNET_HEADER_SIZE + IP_HEADER_SIZE + UDP_HEADER_SIZE)

#define DEFAULT_PACKET_SIZE 1024
#define DEFAULT_NUM_PACKETS 1000
#define DEFAULT_PORT 5000

void udp_sender(const char *dest_ip, int port, int packet_size, int num_packets) {
    int sockfd;
    struct sockaddr_in server_addr;
    char *packet;
    int i;
    
    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    
    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, dest_ip, &server_addr.sin_addr) <= 0) {
        perror("invalid address");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    // Allocate packet buffer
    packet = malloc(packet_size);
    if (!packet) {
        perror("memory allocation failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    // Fill packet with some data (could be random or pattern)
    for (i = 0; i < packet_size; i++) {
        packet[i] = (char)(i % 256);
    }
    
    printf("Sending %d UDP packets of size %d to %s:%d\n", 
           num_packets, packet_size, dest_ip, port);
    
    // Send packets
    for (i = 0; i < num_packets; i++) {
        // Add sequence number at beginning of packet
        *(uint32_t*)packet = htonl(i);
        
        if (sendto(sockfd, packet, packet_size, 0, 
                  (const struct sockaddr *)&server_addr, 
                  sizeof(server_addr)) < 0) {
            perror("sendto failed");
            break;
        }
        
        // Small delay if needed (can be removed for maximum throughput)
        // usleep(1000);
    }
    
    free(packet);
    close(sockfd);
    
    printf("Sent %d packets\n", i);
}


void udp_receiver(int port, int payload_size) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char *packet;
    uint64_t total_payload_bytes = 0;
    uint64_t total_transmitted_bytes = 0;
    uint32_t expected_seq = 0;
    uint32_t seq;
    int lost_packets = 0;
    
    // Timing and measurement variables
    struct timeval start_time, current_time, prev_packet_time;
    double elapsed_seconds;
    
    // Jitter statistics
    double sum_jitter = 0.0;
    double sum_jitter_squared = 0.0;
    double avg_jitter = 0.0;
    double jitter_stddev = 0.0;
    int jitter_samples = 0;
    double prev_arrival_diff = 0.0;
    
    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    packet = malloc(payload_size);
    if (!packet) {
        perror("memory allocation failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("Starting UDP receiver on port %d\n", port);
    printf("Payload size: %d bytes \n\n", payload_size);
    
    client_len = sizeof(client_addr);
    
    // Set up signal handler for clean termination
    
    // Wait for first packet to start measurement
    printf("Waiting for first packet...\n");
    int n = recvfrom(sockfd, packet, payload_size, 0,
                    (struct sockaddr *)&client_addr, &client_len);
    if (n < 0) {
        if (errno == EINTR) exit(0);
        perror("recvfrom failed");
        exit(0);
    }
    
    gettimeofday(&start_time, NULL);
    prev_packet_time = start_time;
    printf("Measurement started\n");
  
    
    // Main measurement loop
    while (total_payload_bytes<99999*1024){
        struct timeval timeout = {1, 0};
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        
        int n = recvfrom(sockfd, packet, payload_size, 0,
                        (struct sockaddr *)&client_addr, &client_len);
        if (n < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) continue;
            if (errno == EINTR) break;
            perror("recvfrom failed");
            break;
        }
        
        gettimeofday(&current_time, NULL);
        elapsed_seconds = (current_time.tv_sec - start_time.tv_sec) +
                         (current_time.tv_usec - start_time.tv_usec) / 1000000.0;
        
  
        
        // Calculate inter-arrival time (μs)
        double current_arrival_diff = (current_time.tv_sec - prev_packet_time.tv_sec) * 1000000.0 +
                                    (current_time.tv_usec - prev_packet_time.tv_usec);
        
        // Calculate jitter (absolute difference from previous inter-arrival)
        if (prev_arrival_diff > 0) {
            double current_jitter = fabs(current_arrival_diff - prev_arrival_diff);
            jitter_samples++;
            sum_jitter += current_jitter;
            sum_jitter_squared += current_jitter * current_jitter;
            
            // Update running statistics
            avg_jitter = sum_jitter / jitter_samples;
            if (jitter_samples > 1) {
                jitter_stddev = sqrt((sum_jitter_squared - (sum_jitter * sum_jitter) / jitter_samples) / (jitter_samples - 1));
            }
        }
        
        prev_arrival_diff = current_arrival_diff;
        prev_packet_time = current_time;
        
        // Process sequence number
        seq = ntohl(*(uint32_t*)packet);
        if (seq != expected_seq) {
            lost_packets += (seq - expected_seq);
            expected_seq = seq + 1;
        } else {
            expected_seq++;
        }
        
        total_payload_bytes += n;
        total_transmitted_bytes += n + TOTAL_HEADER_SIZE;
    }
    
    // Final calculations and output
    gettimeofday(&current_time, NULL);
    elapsed_seconds = (current_time.tv_sec - start_time.tv_sec) +
                     (current_time.tv_usec - start_time.tv_usec) / 1000000.0;
    
    printf("\n=== Measurement Results ===\n");
    printf("Duration:               %.3f seconds\n", elapsed_seconds);
    printf("Total payload:          %lu bytes\n", total_payload_bytes);
    printf("Total transmitted:      %lu bytes\n", total_transmitted_bytes);
    printf("Packet loss:            %d (%.4f%%)\n", lost_packets, 
          (lost_packets * 100.0) / (total_payload_bytes / payload_size + lost_packets));
    
    if (elapsed_seconds > 0) {
        double goodput = (total_payload_bytes * 8) / (elapsed_seconds * 1000000);
        double throughput = (total_transmitted_bytes * 8) / (elapsed_seconds * 1000000);
        
        printf("Goodput (payload):      %.3f Mbps\n", goodput);
        printf("Throughput (total):     %.3f Mbps\n", throughput);
        printf("Protocol overhead:      %.2f%%\n",
              ((total_transmitted_bytes - total_payload_bytes) * 100.0) / total_transmitted_bytes);
        printf("Packet rate:           %.1f pkt/s\n",
              (total_payload_bytes / payload_size) / elapsed_seconds);
    }
    
    if (jitter_samples > 0) {
        printf("\nJitter Statistics:\n");
        printf("Samples:               %d\n", jitter_samples);
        printf("Average jitter:        %.3f μs\n", avg_jitter);
        printf("Jitter std dev:        %.3f μs\n", jitter_stddev);
        printf("Max possible jitter*:  %.3f μs\n", avg_jitter + (3 * jitter_stddev));
        printf("* 3 standard deviations (99.7%% of samples)\n");
    }
    
    free(packet);
    close(sockfd);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage:\n");
        printf("  Sender: %s send <dest_ip> [port] [packet_size] [num_packets]\n", argv[0]);
        printf("  Receiver: %s recv [port] [packet_size]\n", argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "send") == 0) {
        const char *dest_ip = (argc > 2) ? argv[2] : "127.0.0.1";
        int port = (argc > 3) ? atoi(argv[3]) : DEFAULT_PORT;
        int packet_size = (argc > 4) ? atoi(argv[4]) : DEFAULT_PACKET_SIZE;
        int num_packets = (argc > 5) ? atoi(argv[5]) : DEFAULT_NUM_PACKETS;
        
        udp_sender(dest_ip, port, packet_size, num_packets);
    } 
    else if (strcmp(argv[1], "recv") == 0) {
        int port = (argc > 2) ? atoi(argv[2]) : DEFAULT_PORT;
        int packet_size = (argc > 3) ? atoi(argv[3]) : DEFAULT_PACKET_SIZE;
        
        udp_receiver(port, packet_size);
    } 
    else {
        printf("Invalid mode. Use 'send' or 'recv'\n");
        return 1;
    }
    
    return 0;
}


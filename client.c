#include "client.h"
#include "requirements.h"

void start_tcp_client(char *server_address, int port, void *config) {
    int sock;
    struct sockaddr_in server_addr;
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, server_address, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server. Sending config...\n");


    Header header;
    header.msg_type = htons(1);  
    header.msg_length = htons(sizeof(Config)); 
    header.timestamp = htonl(time(NULL));


    if (send(sock, &header, sizeof(Header), 0) < 0) {
        perror("Failed to send header");
        close(sock);
        exit(EXIT_FAILURE);
    }

    
    if (send(sock, config, sizeof(Config), 0) < 0) {
        perror("Failed to send config");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Config sent to server successfully.\n");

    char *message = "Hello, Server!";
    send(sock, message, strlen(message), 0);

    close(sock);
}


void udp_sender(const char *dest_ip, int port, int packet_size, 
    uint64_t bandwidth_bps, double duration_sec) {
    int sockfd;
    struct sockaddr_in server_addr;
    char *packet;
    uint32_t seq = 0;
    struct timeval start_time, current_time;
    double elapsed_seconds;
    uint64_t total_bits_sent = 0;
    uint64_t target_total_bits = bandwidth_bps * duration_sec;
    uint64_t packets_sent = 0;
    uint64_t total_bytes_per_packet;
    double time_per_packet;
    double next_send_time = 0;


    total_bytes_per_packet = packet_size + TOTAL_HEADER_SIZE;

    time_per_packet = (total_bytes_per_packet * 8.0) / bandwidth_bps;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, dest_ip, &server_addr.sin_addr) <= 0) {
        perror("invalid address");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    packet = malloc(packet_size);
    if (!packet) {
        perror("memory allocation failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }


    for (int i = sizeof(uint32_t); i < packet_size; i++) {
        packet[i] = (char)(i % 256);
    }
printf("Sending UDP packets to %s:%d\n", dest_ip, port);
printf("Packet size: %d bytes (+%d headers = %lu total)\n", 
packet_size, TOTAL_HEADER_SIZE, total_bytes_per_packet);
printf("Target bandwidth: %.2f Mbps (%.0f bps)\n", 
bandwidth_bps/1000000.0, (double)bandwidth_bps);
printf("Duration: %.2f seconds\n\n", duration_sec);

gettimeofday(&start_time, NULL);

while (1) {
    gettimeofday(&current_time, NULL);
    elapsed_seconds = (current_time.tv_sec - start_time.tv_sec) +
                (current_time.tv_usec - start_time.tv_usec) / 1000000.0;

    if (elapsed_seconds >= duration_sec) {
        break;
    }

    if (elapsed_seconds >= next_send_time) {
    *(uint32_t*)packet = htonl(seq++);

    if (sendto(sockfd, packet, packet_size, 0, 
            (const struct sockaddr *)&server_addr, 
            sizeof(server_addr)) < 0) {
        perror("sendto failed");
        break;
    }

    packets_sent++;
    total_bits_sent += total_bytes_per_packet * 8;

    next_send_time = packets_sent * time_per_packet;

    if (packets_sent % 1000 == 0) {
        printf("Sent %lu packets (%.2f%% of target bandwidth)\r",
            packets_sent, 
            (double)total_bits_sent * 100.0 / target_total_bits);
        fflush(stdout);
    }
}


}


gettimeofday(&current_time, NULL);
elapsed_seconds = (current_time.tv_sec - start_time.tv_sec) + (current_time.tv_usec - start_time.tv_usec) / 1000000.0;

double actual_bandwidth = (total_bits_sent / elapsed_seconds);
double percentage_of_target = (actual_bandwidth / bandwidth_bps) * 100.0;

printf("\n\n=== Transmission Complete ===\n");
printf("Duration:               %.4f seconds\n", elapsed_seconds);
printf("Packets sent:           %lu\n", packets_sent);
printf("Total payload sent:     %.2f MB\n", 
(packets_sent * packet_size) / (1024.0 * 1024.0));
printf("Actual bandwidth:       %.2f Mbps (%.2f%% of target)\n",
actual_bandwidth / 1000000.0, percentage_of_target);
printf("Average packet rate:    %.2f packets/sec\n",
packets_sent / elapsed_seconds);

free(packet);
close(sockfd);
}


int compare_doubles(const void *a, const void *b) {
    double diff = *(double *)a - *(double *)b;
    return (diff < 0) ? -1 : (diff > 0);
}

void udp_client_duration(const char *server_ip, int port, double duration_sec) {
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    char buffer[1024];

    struct timeval start, end, current;
    double rtt, one_way_delay;
    double delays[MAX_MEASUREMENTS];
    int count = 0;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    gettimeofday(&start, NULL);

    while (1) {
        gettimeofday(&current, NULL);
        double elapsed = (current.tv_sec - start.tv_sec) + (current.tv_usec - start.tv_usec) / 1000000.0;
        if (elapsed >= duration_sec || count >= MAX_MEASUREMENTS)
            break;

        strcpy(buffer, "Ping");

        struct timeval t1, t2;
        gettimeofday(&t1, NULL);

        sendto(sockfd, buffer, strlen(buffer), 0,
               (const struct sockaddr *)&server_addr, sizeof(server_addr));

        recvfrom(sockfd, buffer, sizeof(buffer), 0,
                 (struct sockaddr *)&server_addr, &addr_len);

        gettimeofday(&t2, NULL);

        rtt = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
        one_way_delay = rtt / 2.0;

        delays[count++] = one_way_delay;

        printf("Measurement %d: One-Way Delay ≈ %.3f ms\n", count, one_way_delay);
        usleep(10000);  
    }

    close(sockfd);

    if (count == 0) {
        printf("No measurements taken.\n");
        return;
    }

    qsort(delays, count, sizeof(double), compare_doubles);
    double median;
    if (count % 2 == 0)
        median = (delays[count/2 - 1] + delays[count/2]) / 2.0;
    else
        median = delays[count/2];

    printf("\n=== Summary ===\n");
    printf("Total measurements: %d\n", count);
    printf("Median One-Way Delay: %.3f ms\n", median);
}
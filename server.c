#include "server.h"
#include "requirements.h"


void start_tcp_server(int port, Config *received_config) {
    int server_fd, client_sock;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);
    

    received_config = malloc(sizeof(Config));
    
    // Create TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Bind to port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Start listening
    if (listen(server_fd, 1) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("TCP Server listening on port %d...\n", port);

    // Accept a single client
    client_sock = accept(server_fd, (struct sockaddr *)&address, &addr_len);
    if (client_sock < 0) {
        perror("Accept failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("[Client %d] Connected\n", client_sock);

    // Receive the header first
    Header header;
    if (recv(client_sock, &header, sizeof(Header), 0) <= 0) {
        perror("Failed to receive header");
        close(client_sock);
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Check if the received header has valid length for the config
    int config_length = ntohs(header.msg_length);
    if (config_length > 0) {
        // Receive the Config struct
        if (recv(client_sock, received_config, config_length, 0) <= 0) {
            perror("Failed to receive config");
            close(client_sock);
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        printf("Received config from client:\n");
        if (received_config != NULL) {
            printf("hhhh\n");
            printf("Server: %d, Client: %d\n", received_config->is_server, received_config->is_client);
        
            // Safely print address
            if (received_config->address != NULL) {
                printf("Address: %p\n", (void*)received_config->address);
            } else {
                printf("Address: NULL\n");
            }
        
            printf("Port: %d\n", received_config->port);
            printf("UDP Packet Size: %d, Bandwidth: %d\n",
                   received_config->udp_packet_size,
                   received_config->bandwidth);
        } else {
            printf("Error: received_config is NULL\n");
        }
    }

    // Close the client socket after processing
    close(client_sock);
    close(server_fd);


}

void *handle_client(void *arg) {
    int client_sock = *((int *)arg);
    free(arg);

    char buffer[1024];
    Header header;

    // Receive fixed-size header
    if (recv(client_sock, &header, sizeof(Header), 0) <= 0) {
        perror("Header reception failed");
        close(client_sock);
        return NULL;
    }

    // Convert to host byte order
    header.msg_type = ntohs(header.msg_type);
    header.msg_length = ntohs(header.msg_length);

    printf("[Client %d] Message Type: %d, Length: %d\n", client_sock, header.msg_type, header.msg_length);

    // Receive payload if present
    if (header.msg_length > 0) {
        recv(client_sock, buffer, header.msg_length, 0);
        buffer[header.msg_length] = '\0';
        printf("[Client %d] Payload: %s\n", client_sock, buffer);
    }

    close(client_sock);
    printf("[Client %d] Disconnected\n", client_sock);
    return NULL;
}

void udp_receiver(int port, int payload_size, double duration_sec) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char *packet;
    uint64_t total_payload_bytes = 0;
    uint64_t total_transmitted_bytes = 0;
    uint32_t expected_seq = 0;
    uint32_t seq;
    int lost_packets = 0;

    struct timeval start_time, current_time, prev_packet_time;

    // Jitter stats
    double sum_jitter = 0.0, sum_jitter_squared = 0.0;
    double avg_jitter = 0.0, jitter_stddev = 0.0;
    int jitter_samples = 0;
    double prev_arrival_diff = 0.0;

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Non-blocking mode
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
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
    printf("Payload size: %d bytes\n\n", payload_size);

    client_len = sizeof(client_addr);

    printf("Waiting for first packet...\n");

    // Wait for first packet (blocking to start timing accurately)
    while (1) {
        int n = recvfrom(sockfd, packet, payload_size, 0,
                         (struct sockaddr *)&client_addr, &client_len);
        if (n > 0) break;
    }

    gettimeofday(&start_time, NULL);
    prev_packet_time = start_time;
    printf("Measurement started\n");

    while (1) {
        gettimeofday(&current_time, NULL);
        double elapsed = (current_time.tv_sec - start_time.tv_sec) +
                         (current_time.tv_usec - start_time.tv_usec) / 1e6;

        if (elapsed >= duration_sec)
            break;

        int n = recvfrom(sockfd, packet, payload_size, 0,
                         (struct sockaddr *)&client_addr, &client_len);

        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(10); 
                continue;
            }
            perror("recvfrom failed");
            break;
        }

        gettimeofday(&current_time, NULL);
        double current_arrival_diff = (current_time.tv_sec - prev_packet_time.tv_sec) * 1e6 +
                                      (current_time.tv_usec - prev_packet_time.tv_usec);

        if (prev_arrival_diff > 0) {
            double jitter = fabs(current_arrival_diff - prev_arrival_diff);
            sum_jitter += jitter;
            sum_jitter_squared += jitter * jitter;
            jitter_samples++;
            avg_jitter = sum_jitter / jitter_samples;

            if (jitter_samples > 1) {
                jitter_stddev = sqrt(
                    (sum_jitter_squared - (sum_jitter * sum_jitter) / jitter_samples) /
                    (jitter_samples - 1));
            }
        }

        prev_arrival_diff = current_arrival_diff;
        prev_packet_time = current_time;

        seq = ntohl(*(uint32_t *)packet);
        if (seq != expected_seq) {
            lost_packets += (seq - expected_seq);
            expected_seq = seq + 1;
        } else {
            expected_seq++;
        }

        total_payload_bytes += n;
        total_transmitted_bytes += n + TOTAL_HEADER_SIZE;
    }

    gettimeofday(&current_time, NULL);
    double elapsed_seconds = (current_time.tv_sec - start_time.tv_sec) +
                             (current_time.tv_usec - start_time.tv_usec) / 1e6;

    printf("\n=== Measurement Results ===\n");
    printf("Duration:               %.3f seconds\n", elapsed_seconds);
    printf("Total payload:          %lu bytes\n", total_payload_bytes);
    printf("Total transmitted:      %lu bytes\n", total_transmitted_bytes);
    printf("Packet loss:            %d (%.4f%%)\n", lost_packets,
           (lost_packets * 100.0) / (total_payload_bytes / payload_size + lost_packets));

    if (elapsed_seconds > 0) {
        double goodput = (total_payload_bytes * 8) / (elapsed_seconds * 1e6);
        double throughput = (total_transmitted_bytes * 8) / (elapsed_seconds * 1e6);

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


uint64_t calculate_total_payload_bytes(int packet_size, 
                                       uint64_t bandwidth_bps, double duration_sec) {
    uint64_t total_bits = bandwidth_bps * duration_sec;
    uint64_t total_bytes = total_bits / 8;

    // Number of full packets that can be sent
    uint64_t packets_to_send = total_bytes / packet_size;

    // Total payload bytes sent (excluding headers)
    return packets_to_send * packet_size;
}
#include <thread>
#include <memory>
#include <vector>
#include <chrono>
#include <iostream>
#include <sstream>

#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/resource.h>

struct ThreadOptions {
    struct sockaddr_in servaddr;
    ssize_t message_size;
    size_t max_loops;
    size_t num_threads;
    double duration_seconds;
};

void work(ThreadOptions options) {
    // Create socket
    int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed...\n");
        return;
    }

    // connect the client socket to server socket
    if (connect(sockfd, (sockaddr*)&options.servaddr, sizeof(options.servaddr)) != 0) {
        printf("connection with the server failed...\n");
        return;
    }

    // Timestamp
    auto start_time = std::chrono::system_clock::now();
    auto duration = std::chrono::milliseconds(int(options.duration_seconds * 1000));
    auto end_time = start_time + duration;

    // Get initial usage
    struct rusage start_usage;
    if (getrusage(RUSAGE_THREAD, &start_usage) != 0) {
        perror("rusage");
        return;
    }

    // Create buffer to send and receive data
    std::vector<char> buffer(options.message_size);

    // Loop forever - until max number of loops or time limits are reached
    ssize_t total_bytes = 0;
    size_t counter = 0;
    while (true) {
        // Check time and loop count limits
        if (counter++ >= options.max_loops) break;
        if (std::chrono::system_clock::now() >= end_time) break;

        // Fill buffer with deterministic pattern
        for (size_t j = 0; j < buffer.size(); ++j) buffer[j] = j;

        // Loop until sending all data
        ssize_t offset = 0;
        while (offset < buffer.size()) {
            ssize_t nb = write(sockfd, &buffer[offset], buffer.size() - offset);
            if (nb < 0) {
                perror("Socket error on write");
                break;
            }
            offset += nb;
        }

        // Loop until receiving all data back from the echo server
        ssize_t bytes_received = 0;
        while (bytes_received < buffer.size()) {
            ssize_t nb =
                read(sockfd, &buffer[bytes_received], buffer.size() - bytes_received);
            if (nb < 0) {
                perror("Socket error on read");
                break;
            }
            bytes_received += nb;
        }
        total_bytes += bytes_received;

        // Verify integrity of data received
        for (size_t j = 0; j < buffer.size(); ++j) {
            if (buffer[j] != char(j)) {
                printf("Socket %d data mismatch got %d  expected %d at position %ld\n",
                       sockfd, buffer[j], char(j), j);
            }
        }
    }

    // Get final timestamp
    end_time = std::chrono::system_clock::now();
    auto elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time)
            .count() /
        1E9;

    // Get final usage
    struct rusage final_usage;
    if (getrusage(RUSAGE_THREAD, &start_usage) != 0) {
        perror("rusage");
        return;
    }

    // Compute stats
    long involuntary_switches = start_usage.ru_nivcsw - final_usage.ru_nivcsw;
    long voluntary_switches = start_usage.ru_nvcsw - final_usage.ru_nvcsw;
    double mbs = 1E-6 * (total_bytes / elapsed);

    // Print stats
    std::ostringstream out;
    out << "Threads," << options.num_threads << ",rate," << mbs << ",volsw,"
        << voluntary_switches << ",invsw," << involuntary_switches << std::endl
        << std::flush;
    std::cout << out.str();

    // Close client socket
    ::close(sockfd);
}

#define PORT 9500

using ThreadPtr = std::shared_ptr<std::thread>;

int main(int argc, char* argv[]) {
    // Accepts one argument (optional) - the number of threads to spawn
    size_t numthreads = 1;
    if (argc > 1) {
        numthreads = ::atoi(argv[1]);
    }

    // Fill out the options for the running thread(s)
    ThreadOptions options;
    memset(&options.servaddr, 0, sizeof(options.servaddr));
    options.servaddr.sin_family = AF_INET;
    options.servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    options.servaddr.sin_port = htons(PORT);
    options.message_size = 128 * 1024;
    options.duration_seconds = 5;
    options.max_loops = 1000;
    options.num_threads = numthreads;

    // Loop spawning threads
    std::vector<ThreadPtr> threads;
    for (int j = 0; j < numthreads; ++j) {
        ThreadPtr th = std::make_shared<std::thread>(work, options);
        threads.push_back(th);
    }

    // Loop joining threads until they all finish
    for (const ThreadPtr& ptr : threads) {
        ptr->join();
    }
}

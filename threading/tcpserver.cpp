#include <thread>
#include <memory>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <mutex>

#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>

const ssize_t RECEIVE_BUFFER_SIZE = 4 * 1024 * 1024;
const int PORT = 9500;

// Handy type shorteners
using ThreadId = std::size_t;
using ThreadPtr = std::shared_ptr<std::thread>;
using ThreadMap = std::unordered_map<ThreadId, ThreadPtr>;
using ThreadIdQueue = std::vector<ThreadId>;

// For synchronizing threads with main thread
std::mutex map_lock;
ThreadMap thread_map;
ThreadIdQueue thread_done;

// Echo thread body
void echo(size_t id, int sockfd) {
    std::vector<char> buffer(RECEIVE_BUFFER_SIZE);
    ssize_t total_bytes = 0;
    while (true) {
        // Receive as much data as possible
        ssize_t bytes_received = ::read(sockfd, buffer.data(), buffer.size());
        if (bytes_received < 0) {
            perror("Socket error on write");
            break;
        }
        if (bytes_received == 0) {
            perror("Socket disconnected");
            break;
        }

        // send that buffer back to client
        ssize_t offset = 0;
        while (offset < bytes_received) {
            ssize_t nb = ::write(sockfd, &buffer[offset], bytes_received - offset);
            if (nb < 0) {
                perror("Socket error on write");
                break;
            }
            offset += nb;
        }
        total_bytes += bytes_received;
    }
    // std::cout << "Thread " << pthread_self() << " processed " << total_bytes << "
    // bytes" << std::endl;

    close(sockfd);

    // Notify main thread that it finished
    std::lock_guard<std::mutex> guard{map_lock};
    thread_done.push_back(id);
}

// Driver function
int main() {
    // Create listen socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    } else
        printf("Socket successfully created..\n");

    // assign IP(any/all), PORT
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Bind socket to IP/Port
    if ((bind(sockfd, (sockaddr*)&servaddr, sizeof(servaddr))) != 0) {
        perror("socket bind failed...\n");
        return 1;
    } else
        printf("Socket successfully binded..\n");

    // Start listening
    if ((listen(sockfd, 5)) != 0) {
        perror("Listen failed...\n");
        return 2;
    } else
        printf("Server listening..\n");

    // Keep a counter as a thread ID
    size_t counter = 0;
    while (true) {
        // Accept connection
        struct sockaddr_in cliaddr;
        socklen_t slen = sizeof(cliaddr);
        int connfd = accept(sockfd, (sockaddr*)&cliaddr, &slen);
        if (connfd < 0) {
            perror("server accept failed...\n");
            continue;
        } else {
            printf("server accept client from %s...\n", inet_ntoa(cliaddr.sin_addr));
        }

        // Spawn thread to serve connection
        std::lock_guard<std::mutex> guard{map_lock};
        thread_map.insert(
            {counter, std::make_shared<std::thread>(echo, counter, connfd)});
        counter += 1;

        // Clear any threads finished
        for (size_t id : thread_done) {
            ThreadPtr th(thread_map[id]);
            thread_map.erase(id);
            th->join();
        }
        thread_done.clear();
    }
}

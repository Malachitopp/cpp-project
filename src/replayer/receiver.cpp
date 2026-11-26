#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>
#include <cstring>
#include <charconv>
#include <string_view>
#include "sbs.hpp"



int main(int argc, char* argv[]) {

    if (argc != 3) {
        std::cerr << "usage: publisher <ip> <port>\n";
        return 1;
    }

    const char* port_end = argv[2] + std::strlen(argv[2]);
    std::uint16_t port {};
    auto [ptr, ec] = std::from_chars(argv[2], port_end, port);
    if (ec != std::errc{} || ptr != port_end) {
        std::cerr << "invalid port: " << argv[2] << '\n';
        return 1;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        std::perror("socket");
        return 1;
    }

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, argv[1], &addr.sin_addr) <= 0) {
        std::cerr << "invalid address: " << argv[1] << '\n';
        return 1;
    }

    if (connect(fd, reinterpret_cast<const sockaddr*> (&addr), sizeof(addr)) == -1) {
        std::perror("connect");
        return 1;
    }

    char buffer[4096];
    ssize_t n {};
    std::string msg;
    long nopt {};
    long msgCount {};

    while ( true ) {
        n = recv(fd, buffer, sizeof(buffer) , 0);
        if ( n <= 0 ) { break; }

        msg.append(buffer, n);

        std::size_t pos = msg.find('\n');
        while ( pos != std::string::npos ) {
            auto line = msg.substr( 0, pos );
            auto message =  parse_sbs(line);
            if (!message) { ++nopt; }
            else { ++msgCount; }

            msg.erase(0, pos + 1);
            pos = msg.find('\n');
        }
    }

    if (n == -1) { std::perror("recv"); }

    std::cout << "parsed: " << msgCount << '\n'
              << "failed: " << nopt << '\n'
              << "total:  " << msgCount + nopt << '\n';

    close(fd);
}

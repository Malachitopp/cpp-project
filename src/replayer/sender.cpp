#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <charconv>
#include <cstring>

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cerr << "usage: replayer <file> <port>\n";
        return 1;
    }

    const char* port_end = argv[2] + std::strlen(argv[2]);
    std::uint16_t port {};
    auto [end, ec] = std::from_chars(argv[2], port_end, port);
    if (ec != std::errc{} || end != port_end) {
        std::cerr << "invalid port: " << argv[2] << '\n';
        return 1;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        std::perror("socket");
        return 1;
    }


    sockaddr_in dest {};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    dest.sin_addr.s_addr = htonl(INADDR_ANY);
    
    int val {1};
    int* ptr = &val;

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, ptr, sizeof(val)) == -1) {
        std::perror("setsockopt");
        return 1;
    }

    if (bind(fd, reinterpret_cast<const sockaddr*> (&dest), sizeof(dest)) == -1) {
        std::perror("bind");
        return 1;
    }

    if (listen(fd, 1) == -1) {
        std::perror("listen");
        return 1;
    }

    int client = accept(fd, nullptr, nullptr);
    if (client == -1) {
        std::perror("accept");
        return 1;
    }

    std::ifstream infile(argv[1]);
    if (!infile) {
        std::perror(argv[1]);
        return 1;
    }

    std::string line;


    while ( std::getline(infile, line, '\n'))
    {
        line.push_back('\n');
        auto bytes = send(client, line.data(), line.size(), 0);
        if (bytes == -1) {
            std::perror("send");
            return 1;
        }


    }
    close(fd);
    close(client);

     

    


}

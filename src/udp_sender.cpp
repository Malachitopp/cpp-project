#include <sys/socket.h> 
#include <cstdio> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <cstring> 
#include <unistd.h> 



constexpr const char* DEST_IP = "127.0.0.1";

int main() { int fd = socket(AF_INET, SOCK_DGRAM, 0); 

    if (fd < 0) { 
        std::perror("socket");
        return 1; 
    }

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(5000);
    if (inet_pton(AF_INET, DEST_IP, &dest.sin_addr) != 1) {
        std::fprintf(stderr, "bad address \n");
        return 1;
    }

    const char* msg = "hello" ;
    ssize_t sent = sendto(fd,
                          msg,
                          strlen(msg),
                          0,
                          // says to treat the pointer as a pointer to a diff type
                          reinterpret_cast<const sockaddr*>(&dest),
                          sizeof(dest));
                          
    if (sent == -1) { std::perror("sendto"); return 1;} 

    close(fd); //gives the socket back 

}

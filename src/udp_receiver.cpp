#include <sys/socket.h> 
#include <cstdio> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <cstring> 
#include <unistd.h> 


int main()
{
    auto fd = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(5000);
    
    if (bind(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != 0){
        std::perror("bind");
        return 1;
    }
    
    //array on the stack to store packet
    char buf[1500];
    sockaddr_in from{};
    socklen_t from_len = sizeof(from); 
    //takes next packet from fd queue and copy its bytes into the buffer
    ssize_t n = recvfrom(fd, buf, sizeof(buf), 0,
                         reinterpret_cast<sockaddr*>(&from), &from_len);
   if (n == -1) {std::perror("recvfrom"); return 1;}
    std::fwrite(buf,1, n, stdout);
    std::printf("\n");

    close(fd); 


}

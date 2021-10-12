/*******************************************
* Group Name  : XXXXXX

* Member1 Name: XXXXXX
* Member1 SIS ID: XXXXXX
* Member1 Login ID: XXXXXX

* Member2 Name: XXXXXX
* Member2 SIS ID: XXXXXX
* Member2 Login ID: XXXXXX
********************************************/

#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <arpa/inet.h>

#define PORT "3360"


using namespace std;

void sigchld_handler(int s) {
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

int server() {
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage client_addr;
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        cerr << "getaddrinfo: " << gai_strerror(rv) << '\n';
        return 1;
    }

    // loop through results and bind to first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            cerr << "server: socket\n";
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            cerr << "setsockopt\n";
            return 1;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            cerr << "server: bind\n";
            continue;
        }
        break;          //if we make it this far, we have successfully bound and can exit the loop
    }


    if (p == NULL) {
        cerr << "server: failed to bind\n";
        return 1;
    }

    size_t BACKLOG = 1;             // only 1 connection at a time
    if (listen(sockfd, BACKLOG) == -1) { 
        cerr << "listen\n";
        return 1;
    }

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        cerr << "sigaction\n";
        return 1;
    }

    void *addr;
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
    char ipstr[INET_ADDRSTRLEN];
    addr = &(ipv4->sin_addr);
    cout << "Welcome to Chat!\nWaiting for a connection on\n" << inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr)) << " port " << PORT << "\n"; 

    return 0;
}

int main(int argc, char* argv[]){

    if (argc == 1) {
        int server_status = server();
        return server_status;
    }

  return 0;
}

#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>
#include <stdlib.h>
#include <iostream>
#include <stdint.h>
#include <arpa/inet.h>
#include <inttypes.h>

#define PORT "3987" // the port client will be connecting to

#define MAXBUFLEN 144
#define MAXMESSAGELENGTH 140


using namespace std;


/*******************************************
* Group Name  : TCP 

* Member1 Name: Trevor Gifford
* Member1 SIS ID: 831952416
* Member1 Login ID: tgifford

* Member2 Name: Kassidy Barram
* Member2 SIS ID: 831495701
* Member2 Login ID: kbarram
********************************************/


void sigchld_handler(int s) {
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

//get socket address 
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr); //IPv4
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr); //IPv6
}

void sendPacket(int new_fd){
    string userInput;
    getline(cin, userInput);
    size_t length;
    length = userInput.size();

    while(length > 140){
        cout << "Error: Input too long." << endl;
        cout << "You: " << flush; 
        getline(cin, userInput);
        length = userInput.size();
    }

    uint16_t versionToHost = 457;
    versionToHost = htons(versionToHost);
    uint16_t lengthOfMessage = htons(length);
    char packetToHost[144];
    memset(packetToHost, 0, 144);

    memcpy(packetToHost, &versionToHost, 2);
    memcpy(&(packetToHost[2]), &lengthOfMessage, 2);
    memcpy(&(packetToHost[4]), userInput.c_str(), length);

    if (send(new_fd, packetToHost, MAXBUFLEN, 0) == -1){
        cerr << "send";
    }

}

void receivePacket(int new_fd){

    int numbytes;
    char buf[MAXBUFLEN];

    if ((numbytes = recv(new_fd, buf, MAXBUFLEN, 0)) == -1) { 
        cerr << "recv";
        exit(1);
    }
        
    char packetFromHost[144];
    memcpy(packetFromHost, buf, numbytes);

    char message[140];

    uint16_t version;
    uint16_t packetLength;
    
    memcpy(&version, packetFromHost, 2);
    memcpy(&packetLength, &(packetFromHost[2]), 2);

    version = ntohs(version);
    packetLength = ntohs(packetLength);

    memcpy(&message, &(packetFromHost[4]), 140);
    memset(packetFromHost,0,144);

    message[MAXMESSAGELENGTH] = '\0'; 

    cout << "Friend: "<< message << endl;

    cout << "You: "; 
    memset(message,0,140);

}

void validatePortandIP(string port, string IP){

    for(size_t i = 0; i < IP.size(); i++){
        if(!((IP[i] >= '0' && IP[i] <= '9') || IP[i] == '.')){
            cerr << "Error found in IP input" << endl;
            exit(1); 
        }
    }

    for(size_t i = 0; i < port.size(); i++){
        if(!(port[i] >= '0' && port[i] <= '9')){
            cerr << "Error found in port input" << endl;
            exit(1); 
        }
    }
}


int server() {
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET_ADDRSTRLEN];
    int rv;


    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    char host[256];
    int hostname = gethostname(host, sizeof(host));

    if (hostname == -1) {
        cerr << "gethostname\n";
        return 1;
    }

    struct hostent *host_entry = gethostbyname(host);
    char *host_ip = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));

    if ((rv = getaddrinfo(host_ip, PORT, &hints, &servinfo)) != 0) {
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

    cout << "Welcome to Chat!\nWaiting for a connection on\n" 
    << inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr)) << 
    " port " << PORT << endl;


     while(1) { // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size); 
     
        if (new_fd == -1) {
            cerr << "accept";
        continue; 
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
       
        cout << "Found a friend! " << "You receive first. " << endl;
        
        if (!fork()) { // this is the child process 
            close(sockfd); // child doesn't need the listener 

            while(1){

                receivePacket(new_fd);
                sendPacket(new_fd);

            }
        }
        close(new_fd); // parent doesn't need this }
    }

    return 0;
}


void client(string port, string IP){
    int sockfd;
    struct addrinfo hints, *servinfo, *p; 
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(IP.c_str(), port.c_str(), &hints, &servinfo)) != 0) {
        cerr <<  "getaddrinfo: %s\n", gai_strerror(rv);
        exit(1);
    }

    // loop through all the results and make a socket
    cout << "Connecting to server..." << endl;

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            cerr << "client: socket" << endl; 
            continue;
        }
       
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) { 
            close(sockfd);
            cerr << "client: connect" << endl;
            continue;
        }
        
        break; 
    }

    if (p == NULL) {
        cerr << "client: failed to connect\n"; 
        exit(2);
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);

    cout << "Connected! \n" << "Connected to a friend!" << " You send first." << endl;
    cout << "You: " << flush;

    while(1){

        sendPacket(sockfd);
        receivePacket(sockfd);

    }
    close(sockfd);
}


int main(int argc, char* argv[]){
    int c;
    string port;
    string IP;

    if (argc == 1) {
        int server_status = server();
        return server_status;
    }

    if (argc == 4) {
        cerr << "Invalid Argument Number " << endl;
        return 1;
    }

    while ((c = getopt(argc, argv, "psh")) != -1){

        if(c != 'p' && c != 's' && c != 'h' && c != -1){
            cerr << "Invalid input! \n";
            exit(1);
        }

        if(c == 'h'){
            if(argc != 2){
                cerr << "invalid input!" << endl;
                return 1; 
            }
            cout << "To use the chat program first start the server by running the command: ./chat \n";
            cout << "Once the server is running, start the client by using the command: ./chat -p PORT -s IP \n";
            cout << "Then you will be able to chat with a friend! \n";
            return 0;
        }

        if(c == 'p'){
            if(argc != 5){
                cerr << "Invalid input! \n";
                exit(1);
            }
            port = argv[optind];
        }

        if(c == 's'){
            if(argc != 5){
                cerr << "Invalid input! \n";
                exit(1);
            }
            IP = argv[optind];
        }
    }

    validatePortandIP(port, IP);
    client(port, IP);

    return 0;
}


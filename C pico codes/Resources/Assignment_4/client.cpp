#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 7777
#define MAX_BUFFER 512 

using namespace std;

void printPackageByte(const vector<uint8_t>& package){
    //Made to print the different values of the packet regardless of their type
    cout << "Packet in bytes: ";
    for(auto byte: package){
        printf("%02X ", byte);
    }
    cout << endl;
    return;
}

int main(){
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0); // creates socket
    struct sockaddr_in serverAddress;
    uint8_t buffer[MAX_BUFFER];
    socklen_t addrLen = sizeof(serverAddress);
    string message = "hello Server";

    if (client_socket == -1){
        perror("Errors occurred while creating socket\n Termination program");
        exit(EXIT_FAILURE);
    }

    memset(buffer, 0, MAX_BUFFER); // resets buffer (just in case)
    
    // Set up intermediate host address
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);

    cout << "Sending message to intermediate" << endl;
    cout << message << "\n" << endl;
    sendto(client_socket, message.c_str(), message.size(), 0, (struct sockaddr*)&serverAddress, addrLen);
    ssize_t received = recvfrom(client_socket, buffer, MAX_BUFFER, 0, (struct sockaddr*)&serverAddress, &addrLen);

    if (received > 0){
        buffer[received] = '\0';
        cout << "Client has received info from Intermediate" << endl;
        cout << "Received: " << buffer << endl;
    }
    else{
        cout << "Message was not received by client" << endl;
    }

    close(client_socket);
    return 0;
}
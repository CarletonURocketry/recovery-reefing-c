#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#define SERVER_PORT 8888
#define INTERMEDIATE_PORT 7777
#define MAX_BUFFER 512

using namespace std;

int main(){
    int server_socket = socket(AF_INET, SOCK_DGRAM, 0); // creates socket
    struct sockaddr_in serverAddress, intermediateAddress;
    uint8_t buffer[MAX_BUFFER];
    socklen_t addrLen = sizeof(intermediateAddress);

    if (server_socket == -1){
        perror("Errors occurred while creating socket\n Termination program");
        exit(EXIT_FAILURE);
    }
    
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if(bind(server_socket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
        perror("Error binding server socket");
        exit(EXIT_FAILURE);      
    }
    else{
        cout << "Server port binded and running" << endl;
    }

    while(true){
        ssize_t received = recvfrom(server_socket, buffer, MAX_BUFFER, 0, (sockaddr*)&intermediateAddress, &addrLen);

        if (received == -1){
            perror("Error receiving request from intermediate server");
            exit(EXIT_FAILURE);      
        }
        else{
            cout << "Recieved a message by intermediate host" << endl;
            cout << received << endl;
        }

        vector<uint8_t> server_response;

        server_response.push_back(0x00);
        server_response.push_back(0x04);
        server_response.insert(server_response.end(), buffer, buffer + received);

        if(sendto(server_socket, server_response.data(), server_response.size(), 0, (struct sockaddr*)&intermediateAddress, addrLen) < 0){
            perror("Error sending response to intermediate server");
            exit(EXIT_FAILURE);              
        }
        else{
            cout << "Reply has been sent to intermediate host" << endl;
            cout << server_response.data();
        }
    }

    close(server_socket);
    return 0;
}
#include <iostream>
#include <vector>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define LISTEN_PORT 2023
#define SERVER_PORT 2069

#define MAX_BUFFER 512 


using namespace std;

void printPackageByte(const vector<uint8_t>& packet){
    //Made to print the different values of the packet regardless of their type
    cout << "Packet in bytes: ";
    for(auto byte: packet){
        printf("%02X ", byte);
    }
    cout << endl;
    return;
}

int main(){
    //Create datagram socket to use to receive from client
    int receiveSock = socket(AF_INET, SOCK_DGRAM, 0);
    int serverSock = socket(AF_INET, SOCK_DGRAM, 0); // creates sockets
    struct sockaddr_in receiveAddress, serverAddress, clientAddress, serverRecvAddress; // meant to hold the sockets that were both listening to and sending to
    uint8_t buffer[MAX_BUFFER];
    socklen_t addressLen = sizeof(clientAddress);

    if(receiveSock < 0){
        perror("Error creating receive socket");
        exit(EXIT_FAILURE);
    }

    if(serverSock < 0){
        perror("Error creating send socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if(setsockopt(receiveSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        perror("Error setting SO_REUSEADDR option");
        close(receiveSock);
        close(serverSock);
        exit(EXIT_FAILURE);
    }


    //Receive information from client
    //Create socket for the information to come t0
    memset(&receiveAddress, 0, sizeof(receiveAddress));
    receiveAddress.sin_family = AF_INET;
    receiveAddress.sin_port = htons(LISTEN_PORT);
    receiveAddress.sin_addr.s_addr = INADDR_ANY; 

    //create bind statement to bind re ceiving to port 23
    if(bind(receiveSock, (const sockaddr*)&receiveAddress, sizeof(receiveAddress)) < 0){
        perror("Error receiving info from client\nTerminating now");
        close(receiveSock);
        close(serverSock);
        exit(EXIT_FAILURE);
    }
    
    memset(&serverRecvAddress, 0, sizeof(serverRecvAddress));
    serverRecvAddress.sin_family = AF_INET;
    serverRecvAddress.sin_port = htons(0);
    serverRecvAddress.sin_addr.s_addr = INADDR_ANY;

    if(bind(serverSock, (const sockaddr*)&serverRecvAddress, sizeof(serverRecvAddress)) < 0){
        perror("Error binding server socket");
        close(receiveSock);
        close(serverSock);
        exit(EXIT_FAILURE);
    }

    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); 

    cout << "Intermediate Host is running" << endl;

    while(true){
        //Receive information from client
        ssize_t receivedLen = recvfrom(receiveSock, buffer, MAX_BUFFER, 0, (sockaddr*)&clientAddress, (unsigned int*)&addressLen);
        if(receivedLen < 0){
            perror("Error receiving information from client \nTerminating");
            exit(EXIT_FAILURE);
        }
        //print information from client both as string and bytes

        vector<uint8_t> sendPackage(buffer, buffer + receivedLen);
        cout << "Information from client received" << endl;
        printPackageByte(sendPackage);

        memset(buffer, 0, MAX_BUFFER);

        //Form packet to send containing exactly what was received
        //This should jsut be "receivedPackage"
        if(sendto(receiveSock, sendPackage.data(), sendPackage.size(), 0, (const sockaddr*)&serverAddress, addressLen) < 0){
            perror("Error sending packet \nterminating program");
            close(receiveSock);
            close(serverSock);
            exit(EXIT_FAILURE);
        } 

        cout <<"Package received from client has now been sent to server" << endl;
        //Host prints information

        // Host sends packet on send/receive socket to port 69, wait for response
        ssize_t received = recvfrom(receiveSock, buffer, MAX_BUFFER, 0, (sockaddr*)&serverRecvAddress, &addressLen);
        if(received < 0){
            perror("Error receiving resposne\nterminating program");
            close(serverSock);
            exit(EXIT_FAILURE);
        }

        vector<uint8_t> server(buffer, buffer + received);
        //print received infromation
        cout << "Received a response from server" << endl;
        printPackageByte(server);

        //form packet to send back to client
        if(sendto(receiveSock, server.data(), server.size(), 0, (const sockaddr*)&clientAddress, addressLen) < 0){
            perror("Error sending information back to client \nTerminating");
            close(receiveSock);
            exit(EXIT_FAILURE);
        }

        cout <<"Information has been sent to client\n" << endl;
        //create datagram socket (again)
        //print info being sent
    }
    close(receiveSock);
    close(serverSock);
    return 0;
}
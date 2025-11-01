#include <iostream>
#include <vector>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVER_PORT 2069
#define MAX_BUFFER 512

using namespace std;

void printPackageByte (const vector<uint8_t>& packet){
    //Made to print the different values of the packet regardless of their type
    cout << "Packet in bytes: ";
    for(auto byte: packet){
        printf("%02X ", byte);
    }
    cout << endl;
    return;
} 

void parsePackage(vector<uint8_t> package){
        // if read request send back 0301
        // if write request send back 0400
        
        //print response packet info
        //create datagram socket to use for response
        // send packet via new socket to port received from
        //close socket that was created      
        
}      

int main() {
    //create datagramSocket to use to receive
    int receiveSock; // creates sockets
    struct sockaddr_in receiveAddress, intermediateAddress; // meant to hold the sockets that were both listening to and sending to
    uint8_t buffer[MAX_BUFFER];
    socklen_t addressLen = sizeof(intermediateAddress);
    vector<uint8_t> returnPackage;

    receiveSock  = socket(AF_INET, SOCK_DGRAM, 0);
    if(receiveSock < 0){
        perror("Error creating server socket");
        close(receiveSock);
        return 1;
    }

    int opt = 1;
    if(setsockopt(receiveSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        perror("Error setting SO_REUSEADDR option");
        close(receiveSock);
        return 1;
    }

    memset(&receiveAddress, 0, sizeof(receiveAddress));
    receiveAddress.sin_family = AF_INET;
    receiveAddress.sin_port = htons(SERVER_PORT);
    receiveAddress.sin_addr.s_addr = INADDR_ANY; 

    if(bind(receiveSock, (const sockaddr*)&receiveAddress, sizeof(receiveAddress)) < 0){
        perror("Error binding server socket\nTerminating now");
        close(receiveSock);
       exit(EXIT_FAILURE);
    }   

    cout << "Server is now running" << endl;

    //wait until request is sent
    while(true){
        //read request
        ssize_t received = recvfrom(receiveSock, buffer, MAX_BUFFER, 0, (sockaddr*)&intermediateAddress, (unsigned int*)&addressLen);
        if(received < 0){
            perror("Error receiving resposne\nterminating program");
            close(receiveSock);
            exit(EXIT_FAILURE); // if invalid, return
        }
        //print info as a string and as bytes
        vector<uint8_t> receivedPackage(buffer, buffer + received); // puts all parts of the package into a vector list that can be unpacked
        cout << "Package received from intermediate host" << endl;
        printPackageByte(receivedPackage);

        uint16_t operation = ntohs(*(uint16_t*)buffer);
        if(received < 4){
            perror("Invalid request");
            exit(EXIT_FAILURE);
        }

        cout << "Recieved TFTP Request (Opcode: " << operation << ")" << endl;
        if(operation == 1){ // read data request
            //returnPacket = {0x00, 0x03, 0x00, 0x01};
            returnPackage.push_back(0x00);
            returnPackage.push_back(0x03);
            returnPackage.push_back(0x00);
            returnPackage.push_back(0x01);

            std::cout << "Read request received, return packet created" << std::endl;
        }
        else if(operation == 2) {// write data request
           // returnPacket = {0x00, 0x04, 0x00, 0x00};
           returnPackage.push_back(0x00);
           returnPackage.push_back(0x04);
           returnPackage.push_back(0x00);
           returnPackage.push_back(0x00);
            std::cout << "Write request received, return packet created" << std::endl;
        }
        else{
            std::cout << "invalid request" << std::endl;
            exit(EXIT_FAILURE);
        }

        printPackageByte(returnPackage);

        if(sendto(receiveSock, returnPackage.data(), returnPackage.size(), 0, (const sockaddr*)&intermediateAddress, addressLen) < 0){
            perror("Error sending information back to client \nTerminating");
            close(receiveSock);
            exit(EXIT_FAILURE);
        }
        cout << "Information send from Server to Intermediate" << endl;
        returnPackage.pop_back();
        returnPackage.pop_back();
        returnPackage.pop_back();
        returnPackage.pop_back();
    }
    close(receiveSock);
    return 0;
}


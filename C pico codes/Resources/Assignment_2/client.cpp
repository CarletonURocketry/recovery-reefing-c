#include <iostream>
#include <vector>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <algorithm>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 2023
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
    //create datagramsocket to both send and receive requests
    int skt = socket(AF_INET, SOCK_DGRAM, 0); // creates socket
    struct sockaddr_in serverAddress;
    uint8_t buffer[MAX_BUFFER];
    socklen_t addrLen = sizeof(serverAddress);
    vector<string> fileNames = {"file1.txt", "file2.txt", "file3.txt", "file4.txt", "file5.txt",
                                        "file6.txt", "file7.txt", "file8.txt", "file9.txt", "file10.txt"};

    if (skt == -1){
        perror("Error occurred while creating socket\n Termination program");
        exit(EXIT_FAILURE);
    }

    memset(buffer, 0, MAX_BUFFER); // resets buffer (just in case)
    
    // Set up intermediate host address
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);

    string packageMode = "octet"; // mode that will be bytes later

    //Read and write requests, make even numbers read and make odd numbers write
    for_each(fileNames.begin(), fileNames.end(), [packageMode, skt, buffer, serverAddress, addrLen](auto &y) {
        vector<uint8_t> package;

        static int index = 1;
        if(index > 10){
            package.push_back(0xFF);
            package.push_back(0xFF);
        }
        else if((index % 2) == 0){ //divisible by 2 therefore will be write (even numbers are write)
            package.push_back(0x00);
            package.push_back(0x02);
        }
        else{ //has a remainder therefore will be read (odd numbers are read)
            package.push_back(0x00);
            package.push_back(0x01);
        }

        //Push filename after converting to bytes
        string file = (index < 10) ? y : "invalid.txt";
        package.insert(package.end(), file.begin(), file.end());

        //Push 0 byte
        package.push_back(0x00);

        //Push Mode
        string mode = packageMode;
        package.insert(package.end(), mode.begin(), mode.end());

        //Push last 0 byte
        package.push_back(0x00);

        //Print information before byte conversion
        cout << "Sending request " << (index) << ": " 
             << (((index % 2) == 1) ? "READ" : ((index % 2) == 0) ? "WRITE" : "INVALID")
             << " " << y << " " << mode << endl;
        
        //Print information After byte conversion
        printPackageByte(package);

        //Sends packet to port 23/intermediate host
        if(sendto(skt, package.data(), package.size(), 0, (const sockaddr*)&serverAddress, addrLen) < 0){
            perror("Error sending packet \nterminating program");
            close(skt);
            exit(EXIT_FAILURE);
        }

        //Receive info back from intermediate host
        ssize_t received = recvfrom(skt, (void*)buffer, MAX_BUFFER, 0, (sockaddr*)&serverAddress, (unsigned int*)&addrLen);
        if(received < 0){
            perror("Error receiving resposne\nterminating program");
            close(skt);
            exit(EXIT_FAILURE);
        }

        vector<uint8_t> intermediate(buffer, buffer + received);
        cout << "Packet Received from Intermediate" << endl;

        //Print received data 
        printPackageByte(intermediate);
        cout << endl;
        index++;
    });
    close(skt);
    return 0;
}
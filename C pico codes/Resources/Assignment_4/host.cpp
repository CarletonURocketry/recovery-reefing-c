#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

#define CLIENT_PORT 9998
#define INTERMEDIATE_PORT 7777
#define SERVER_PORT 8888
#define SERVER_IP "127.0.0.1"
#define MAX_BUFFER 512

using namespace std;

void listen_check(int receive_socket, struct sockaddr_in &receive_address, struct sockaddr_in &send_address){
    char buffer[MAX_BUFFER];
    socklen_t address_len = sizeof(receive_address);

    while(true) { //should effectively run forever
        ssize_t received = recvfrom(receive_socket, buffer, MAX_BUFFER, 0, (sockaddr*)&receive_address, &address_len);
        cout << received << endl;
        if (received == -1){
            perror("Error occurred while creating socket\n Termination program");
            exit(EXIT_FAILURE);
        }
        else{
            cout << "Message " << received << " has been received, forwarding now\n" << endl;
        }

        if(sendto(receive_socket, buffer, received, 0, (struct sockaddr*)&send_address, sizeof(send_address)) < 0){
            perror("Error occurred while forwarding message\n Termination program");
            exit(EXIT_FAILURE);           
        }

        ssize_t reply = recvfrom(receive_socket, buffer, MAX_BUFFER, 0, (sockaddr*)&receive_address, &address_len);
        if (reply == -1){
            perror("Error occurred while attemtping to receive reply\n Termination program");
            exit(EXIT_FAILURE);
        }
        else{
            cout << "Reply Message " << reply << " has been received, sending back\n" << endl;
        }  
        cout << "Reply Message " << reply << " has been received, sending back\n" << endl;
          
        if(sendto(receive_socket, buffer, reply, 0, (struct sockaddr*)&send_address, sizeof(send_address)) < 0){
            perror("Error occurred while sending message back to sender\n Termination program");
            exit(EXIT_FAILURE);           
        }
    }
}

int main(){
    int intermediate_host_socket = socket(AF_INET, SOCK_DGRAM, 0); // creates socket
    struct sockaddr_in clientAddress, intermediateAddress, serverAddress;

    if (intermediate_host_socket < 0){
        perror("Error creating intermediate socket");
        exit(EXIT_FAILURE);
    }
    
    // Set up intermediate host address
    memset(&intermediateAddress, 0, sizeof(intermediateAddress));
    intermediateAddress.sin_family = AF_INET;
    intermediateAddress.sin_port = htons(INTERMEDIATE_PORT);
    intermediateAddress.sin_addr.s_addr = INADDR_ANY;

    if(bind(intermediate_host_socket, (struct sockaddr*)&intermediateAddress, sizeof(intermediateAddress)) < 0){
        perror("Error binding intermediate socket");
        exit(EXIT_FAILURE);      
    }
    else{
        cout << "Intermediate port binded and running" << endl;
    }

    // Set up server
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP); 

    // Set up client
    memset(&clientAddress, 0, sizeof(clientAddress));
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_port = htons(CLIENT_PORT);
    clientAddress.sin_addr.s_addr = INADDR_ANY;

    cout << "Creating threads" << endl;
    thread client_to_server(listen_check, intermediate_host_socket, ref(clientAddress), ref(serverAddress));
    thread server_to_client(listen_check, intermediate_host_socket, ref(serverAddress), ref(clientAddress));

    client_to_server.join();
    server_to_client.join();

    close(intermediate_host_socket);
    return 0;
}
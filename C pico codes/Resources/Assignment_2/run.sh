#compile all files
g++ server.cpp -o server
g++ intermediate.cpp -o intermediate
g++ client.cpp -o client


./server & ./intermediate & ./client

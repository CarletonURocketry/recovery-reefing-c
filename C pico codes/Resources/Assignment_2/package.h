#ifndef PACKAGE_H
#define PACKAGE_H

#include <string>
using namespace std;

/*
    @brief Represents the package that will be getting sent to and from different classes
    Will store:
        1. two bytes indicating read or write
        2. filename converted from string to bytes
        3. normal 0 byte
        4. mode from string to bytes
        5. normal 0 byte

*/

struct Package{
    int readOrWrite;
    string fileName;
    int byte = 0;
    string mode;

    /*
        @brief constructs a package with given parameters

        @param rw for read or write 
        @param f for fileName
        @param m for mode
    */
   Package(int rw, string f, string m) : readOrWrite(rw), fileName(f), mode(m) {}
};


#endif
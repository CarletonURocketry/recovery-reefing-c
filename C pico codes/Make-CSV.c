#include <stdio.h>


int main(){
    FILE *fpt;
    char filename[] = "NewFile.csv";

    fpt = fopen(filename, "w+");

    fprintf(fpt, "This is a test\n");

    fclose(fpt);
    return 0;
}

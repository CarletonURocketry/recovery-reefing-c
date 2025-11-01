#include <stdio.h>
#include <time.h>

int main(){
    FILE *fpt;
    char filename[] = "NewFile.csv";
    time_t curr_time;

    fpt = fopen(filename, "w+");
    time(&curr_time); // we may have to do this every time we get a new time to clock
    

    fprintf(fpt, "Current Time: %s", ctime(&curr_time));

    fclose(fpt);
    return 0;
}

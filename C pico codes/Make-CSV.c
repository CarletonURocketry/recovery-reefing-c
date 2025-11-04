#include <stdio.h>
#include <string.h>
#include <datetimeapi.h>
#include <time.h>

void record_data(FILE *fpt, char string[]){
    time_t curr_time; // create time variable
    time(&curr_time); // get current time
    char *time = ctime(&curr_time); // turn it into a string

    if(time[strlen(time) - 1] = "\n"){ // if time has a \n at the end it will make the csv look ugly, this fixes that
        time[strlen(time) - 1] = "\0";
    }

    fprintf(fpt, "Current Time: %s --------------- %s", time, string); // add line of text to CSV
    return;
}

int main(){
    FILE *fpt; // creates CSV file variable
    char filename[] = "NewFile.csv"; // name of CSV file
    time_t curr_time;

    fpt = fopen(filename, "w+"); // Attaches CSV file to CSV variable
    
    record_data(fpt, "Test has worked"); // Function to call whenever data needs to be added to csv

    fclose(fpt);
    return 0;
}

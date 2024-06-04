#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char **argv){
    if(argc != 2){
        fprintf(2, "Usage:sleep number (number>=0)\n");
        exit(1); 
    }else{
        int times = atoi(argv[1]);
        if(times>0){
            sleep(times);
            printf("(nothing happens for a little while)");
        }else{
            fprintf(2, "Usage:let '0'<number[i]<'9'\n");
            exit(1);
        }
    }

    exit(0);
}
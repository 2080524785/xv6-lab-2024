#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[]){

    char* new_argv[MAXARG];
    for(int i=1;i<argc;i++){
        new_argv[i-1] = argv[i];
    }
    int new_argv_num = argc-1;
    char c;
    char buf[128], *argv_buf=buf;
    new_argv[new_argv_num++] = argv_buf;
    while(read(0, &c, sizeof(char))){
        if(c==' '){
            *argv_buf++='\0';
            new_argv[new_argv_num++] = argv_buf;
        }else if(c=='\n'){
            *argv_buf='\0';
            new_argv[new_argv_num] = 0;
            if(fork()==0){
                exec(new_argv[0], new_argv);
                exit(0);
            }else{
                wait((int *)0);
                argv_buf = buf;
                new_argv_num = argc;
            }
        }else{
            *argv_buf++=c;
        }
    }
    

    exit(0);
}
#include "kernel/types.h"
#include "user/user.h"

int main(){
    int p1[2], p2[2];
    pipe(p1);
    pipe(p2);
    char buf[8];
    if(fork()==0){
        close(p1[1]);
        read(p1[0], buf, 4);
        close(p1[0]);
        printf("%d: received %s\n", getpid(), buf);
        close(p2[0]);
        write(p2[1], "pong", strlen("pong"));
        close(p2[1]);
    }else{
        close(p1[0]);
        write(p1[1], "ping", strlen("ping"));
        close(p1[1]);
        wait((int *)0);
        close(p2[1]);
        read(p2[0], buf, 4);
        close(p2[0]);
        printf("%d: received %s\n", getpid(), buf);
    }
    exit(0);
}
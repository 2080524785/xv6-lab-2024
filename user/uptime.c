#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char **argv) {
    int ut = uptime();
    fprintf(1,"up %d ticks\n", ut);
    exit(0);
}
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "server.h"

int main(int argc, char** argv) {
    // Your code here
    int p;
    char * gevent = "gevent";

    mkfifo(gevent, 666);

    p = open(gevent, O_RDONLY);
    char message[2048];
    read(p,message,2048);
    printf("%s", message);
    close(p);

    return 0;
}

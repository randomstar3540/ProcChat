#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "server.h"

int main(int argc, char** argv) {
    // Your code here
    int p[2];
    char * rd_pipe = "_RD";
    char * wr_pipe = "_WR";

    mkfifo(rd_pipe, 666);
    mkfifo(wr_pipe, 666);

    p[0] = open(rd_pipe, O_RDONLY);
    p[1] = open(wr_pipe, O_WRONLY);

    close(p[0]);
    close(p[1]);

    return 0;
}

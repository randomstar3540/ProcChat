#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "server.h"

int main(int argc, char** argv) {
    // Your code here
    int pipe[2];
    char * rd_pipe = "_RD";
    char * wr_pipe = "_WR";

    mkfifo(rd_pipe, 666);
    mkfifo(wr_pipe, 666);

    pipe[0] = open(rd_pipe, O_RDONLY);
    pipe[1] = open(rd_pipe, O_WRONLY);

    return 0;
}

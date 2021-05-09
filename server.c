#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "server.h"

int main(int argc, char** argv) {
    int p;
    char * gevent = "gevent";
    mkfifo(gevent, 0666);
    char message[2048];
    char id[256];
    char domain[256];
    char p_RD_name[515];
    char p_WR_name[515];
    char domain_path[257];
    pid_t child;
    while(1){
        p = open(gevent, O_RDONLY);
        read(p,message,2048);
        close(p);

        child = fork();
        if (child == -1) {
            printf("fork failed\n");
            exit(-1);
        }
        if (child) {
            printf("connect!\n"); //parent
        } else {
            break;//child
        }
    }

    // uint16_t tcode = message[0] << 8 | message[1];
    memcpy(id,&message[2],256);
    memcpy(domain,&message[2+256],256);

    sprintf(p_RD_name,"%s/%s_RD",domain,id);
    sprintf(p_WR_name,"%s/%s_WR",domain,id);
    sprintf(domain_path,"%s/",domain);

    struct stat st = {0};
    if (stat(domain_path, &st) == -1) {
        mkdir(domain_path, 0700);
    }
    mkfifo(p_RD_name, 0666);
    mkfifo(p_WR_name, 0666);

    return 0;
}

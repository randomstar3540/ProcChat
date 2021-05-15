#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

#include "server.h"

void say_handler(char *domain, char* self, char* message){
    DIR *dp;
    struct dirent *file;
    struct stat file_stat;
    int p;
    char self_pipe_name[PIPE_NAME_MAX];
    char response[MESSAGE_LEN];

    sprintf(self_pipe_name,"%s_RD",self);

    if((dp = opendir(domain)) == NULL) {
        fprintf(stderr,"cannot open directory: %s\n", domain);
        return;
    }

    chdir(domain);

    while((file = readdir(dp)) != NULL) {

        stat(file->d_name,&file_stat);
        if(!S_ISFIFO(file_stat.st_mode)) {
            continue;
        }
        if(strcmp(&((file->d_name)[5]), "_RD") != 0){
            continue;
        }

        printf("%s\n",file->d_name);

        if(strcmp(file->d_name, self_pipe_name) == 0){
            continue;
        }

        memset(response,0,2048);
        response[0] = RECEIVE;
        strcpy(&response[TYPE_LEN],self);
        memcpy(&response[TYPE_LEN+PIPE_NAME_MAX],
               &message[TYPE_LEN], 1790);

        p = open(file->d_name, O_WRONLY);
        write(p,response,2048);
        close(p);

    }

    chdir("..");
    closedir(dp);
}


int main(int argc, char** argv) {
    int p;
    char * gevent = "gevent";
    mkfifo(gevent, 0666);
    char message[MESSAGE_LEN];

    pid_t child;
    while(1){
        p = open(gevent, O_RDWR);
        read(p,message,MESSAGE_LEN);
        close(p);

        uint16_t tcode = message[1] << 8 | message[0];

        if(tcode != CONNECT){
            continue;
        }

        child = fork();
        if (child == -1) {
            printf("fork failed\n");
            exit(-1);
        }
        if (child) {
            printf("%d\n",child);
        }else{
            break;//child
        }
    }

    char id[PIPE_NAME_MAX];
    char domain[PIPE_NAME_MAX];
    memcpy(id,&message[2],PIPE_NAME_MAX);
    memcpy(domain,&message[2+PIPE_NAME_MAX],PIPE_NAME_MAX);

    char p_RD_name[PIPE_PATH_MAX];
    char p_WR_name[PIPE_PATH_MAX];
    sprintf(p_RD_name,"%s/%s_RD",domain,id);
    sprintf(p_WR_name,"%s/%s_WR",domain,id);

    char domain_path[PIPE_NAME_MAX + 1];
    sprintf(domain_path,"%s/",domain);

    struct stat st = {0};
    if (stat(domain_path, &st) == -1) {
        mkdir(domain_path, 0700);
    }
    mkfifo(p_RD_name, 0666);
    mkfifo(p_WR_name, 0666);
    fprintf(stderr,"%s:%s:SPAWN\n", domain, id);

    while(1){
        p = open(p_WR_name, O_RDWR);
        read(p,message,2048);
        close(p);

        uint16_t tcode = message[1] << 8 | message[0];


        if(tcode == CONNECT){
            continue;
        }
        else if(tcode == SAY){
            fprintf(stderr,"%s:%s:SAY\n", domain, id);
            say_handler(domain,id,message);
        }
        else if(tcode == SAYCONT){
            continue;
        }
        else if(tcode == PING){
            continue;
        }
        else if(tcode == DISCONNECT){
            break;
        }
        else{
            continue;
        }
    }
    printf("disconnect!\n");

    unlink(p_RD_name);
    unlink(p_WR_name);

    return 0;
}

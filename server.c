#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdint.h>
#include <string.h>
#include <poll.h>
#include <time.h>
#include <stdlib.h>
#include <dirent.h>
#include <signal.h>

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
        int name_len = strlen(file->d_name);
        if(strcmp(&((file->d_name)[name_len-3]), "_RD") != 0){
            continue;
        }

        if(strcmp(file->d_name, self_pipe_name) == 0){
            continue;
        }

        memset(response,0,MESSAGE_LEN);
        strcpy(&response[TYPE_LEN],self);
        memcpy(&response[TYPE_LEN+PIPE_NAME_MAX],
            &message[TYPE_LEN], SAY_MSG_LEN);

        response[0] = RECEIVE;

        p = open(file->d_name, O_WRONLY);
        write(p,response,MESSAGE_LEN);
        close(p);

    }

    chdir("..");
    closedir(dp);
}

void saycont_handler(char *domain, char* self, char* message){
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
        int name_len = strlen(file->d_name);
        if(strcmp(&((file->d_name)[name_len-3]), "_RD") != 0){
            continue;
        }

        if(strcmp(file->d_name, self_pipe_name) == 0){
            continue;
        }

        uint8_t ter_byte = message[SAYCONT_TER];

        if(ter_byte != TERMINATION){
            ter_byte = 0;
        }

        memset(response,0,MESSAGE_LEN);
        strcpy(&response[TYPE_LEN],self);
        memcpy(&response[TYPE_LEN+PIPE_NAME_MAX],
            &message[TYPE_LEN], SAYCONT_MSG_LEN);

        response[0] = RECVCONT;
        response[SAYCONT_TER] = ter_byte;

        p = open(file->d_name, O_WRONLY);
        write(p,response,MESSAGE_LEN);
        close(p);

    }

    chdir("..");
    closedir(dp);
}

void handle_sig_usr1(){
    int status;
    pid_t pid;
    sleep(1);

    while((pid = waitpid(-1, &status, WNOHANG|WUNTRACED)) > 0){
        continue;
    }

    return;
}

int main(int argc, char** argv) {
    int p;
    char * gevent = "gevent";
    mkfifo(gevent, 0666);
    char message[MESSAGE_LEN];

    pid_t child;
    pid_t parent_pid;
    parent_pid = getpid();

    signal(SIGUSR1, handle_sig_usr1);
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

    int p_rd;
    int p_wr;
    p_rd = open(p_RD_name, O_RDWR);
    p_wr = open(p_WR_name, O_RDWR);

    struct pollfd npipes[2];
    npipes[0].fd = p_wr;
    npipes[0].events = POLLIN;
    npipes[1].fd = p_rd;
    npipes[1].events = POLLOUT;

    time_t last_ping;
    time_t last_pong;
    time_t now;

    while(1){
        int ret = poll(npipes, 2, 5);
        time(&now);

        if (ret == -1){
            printf("poll error\n");
            exit(1);
        }


        if (npipes[0].revents & POLLIN) {
            read(p_wr,message,2048);

            uint16_t tcode = message[1] << 8 | message[0];

            if(tcode == CONNECT){
                continue;
            }
            else if(tcode == SAY){
                say_handler(domain,id,message);
            }
            else if(tcode == SAYCONT){
                saycont_handler(domain,id,message);
            }
            else if(tcode == PONG){
                time(&last_pong);
                continue;
            }
            else if(tcode == DISCONNECT){
                break;
            }
            else{
                continue;
            }
        }

        if (npipes[1].revents & POLLOUT) {
            int ping_diff = difftime(now, last_ping);
            int pong_diff = difftime(now, last_pong);
            if(ping_diff >= 15){
                char response[MESSAGE_LEN];
                memset(response,0,MESSAGE_LEN);
                response[0] = PING;
                write(p_rd,response,MESSAGE_LEN);
                time(&last_ping);
            }
            if(last_pong >= 15){
                break;
            }
        }
    }
    close(p);

    unlink(p_RD_name);
    unlink(p_WR_name);
    kill(parent_pid,SIGUSR1);

    return 0;
}

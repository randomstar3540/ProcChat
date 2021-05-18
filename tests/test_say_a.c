# include "../server.h"

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

void printdir(char *dir)
{
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    if ((dp = opendir(dir)) == NULL) {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }
    chdir(dir);
    while ((entry = readdir(dp)) != NULL) {
        stat(entry->d_name, &statbuf);
        if(!S_ISFIFO(statbuf.st_mode)) {
            continue;
        }
        printf("%s\n",entry->d_name);
    }
    chdir("../tests");
    closedir(dp);
}


void say(char * wr_pipe, char * content){
    int p;
    char message[2048];
    memset(message,0,MESSAGE_LEN);
    strcpy(&message[2],content);
    message[0] = SAY;

    p = open(wr_pipe, O_WRONLY);
    write(p,message,MESSAGE_LEN);
    close(p);
}

void saycont(char * wr_pipe, char * content, int ter){
    int p;
    char message[2048];
    memset(message,0,MESSAGE_LEN);
    strcpy(&message[2],content);
    message[0] = SAYCONT;

    if(ter == 255){
        message[2047]=255;
    }

    p = open(wr_pipe, O_WRONLY);
    write(p,message,MESSAGE_LEN);
    close(p);
}

void connect(char * id, char * domain){
    int p;
    char message[2048];
    memset(message,0,MESSAGE_LEN);
    strcpy(&message[2], id);
    strcpy(&message[2+256], domain);
    message[0] = CONNECT;

    p = open("../gevent", O_WRONLY);
    write(p,message,MESSAGE_LEN);
    close(p);
}

void disconnect(char * wr_pipe){
    int p;
    char message[2048];
    memset(message,0,MESSAGE_LEN);
    message[0] = DISCONNECT;

    p = open(wr_pipe, O_WRONLY);
    write(p,message,MESSAGE_LEN);
    close(p);
}

void pong(char * wr_pipe){
    int p;
    char message[2048];
    memset(message,0,MESSAGE_LEN);
    message[0] = PONG;

    p = open(wr_pipe, O_WRONLY);
    write(p,message,MESSAGE_LEN);
    close(p);
}

void read_input(char * rd_pipe){
    int p;
    unsigned char message[2048];
    p = open(rd_pipe, O_RDWR);
    read(p,message,2048);
    close(p);

    int counter = 0;
    while (counter < 2048){
        int line = 0;
        while(line < 16){
            int byte = 0;
            while(byte < 2){
                printf("%02.2X", message[counter]);
                counter ++;
                byte++;
                line++;
            }
        }
    }
}

int main(){
    char * domain = "domain2";
    char * id = "first";

    char p_RD_name[PIPE_PATH_MAX];
    char p_WR_name[PIPE_PATH_MAX];
    sprintf(p_RD_name,"../%s/%s_RD",domain,id);
    sprintf(p_WR_name,"../%s/%s_WR",domain,id);


    connect(id,domain);
    sleep(2);
    say(p_WR_name,"HelloHello");
    sleep(1);
    disconnect(p_WR_name);
    return 0;

}

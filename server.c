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

void say_handler(char *domain, char *self, char *message) {
    /*
     * Handle SAY requests from client, reply with RECEIVE
     */
    DIR *dp;
    struct dirent *file;
    struct stat file_stat;
    int p;
    char self_pipe_name[PIPE_NAME_MAX];
    char response[MESSAGE_LEN];

    sprintf(self_pipe_name, "%s_RD", self);

    if ((dp = opendir(domain)) == NULL) {
        fprintf(stderr, "cannot open directory: %s\n", domain);
        return;
    }

    chdir(domain);

    while ((file = readdir(dp)) != NULL) {
        /*
         * Loop over the folder, find FIFOs ended with "_RD"
         */
        stat(file->d_name, &file_stat);
        if (!S_ISFIFO(file_stat.st_mode)) {
            continue;
        }
        int name_len = strlen(file->d_name);
        if (strcmp(&((file->d_name)[name_len - 3]), "_RD") != 0) {
            /*
             * Get the last three char and compare
             */
            continue;
        }

        if (strcmp(file->d_name, self_pipe_name) == 0) {
            /*
             * Check if st is self
             */
            continue;
        }

        /*
         * Create a response message
         */
        memset(response, 0, MESSAGE_LEN);
        strcpy(&response[TYPE_LEN], self);
        memcpy(&response[TYPE_LEN + PIPE_NAME_MAX],
               &message[TYPE_LEN], SAY_MSG_LEN);

        response[0] = RECEIVE;

        /*
         * Write the message, in blocking mode
         */
        p = open(file->d_name, O_WRONLY);
        if (p == -1) {
            return;
        }
        write(p, response, MESSAGE_LEN);
        close(p);

    }

    chdir("..");
    closedir(dp);
}

void saycont_handler(char *domain, char *self, char *message) {
    /*
     * Handle SAYCONT requests from client, reply with RECVCONT
     */
    DIR *dp;
    struct dirent *file;
    struct stat file_stat;
    int p;
    char self_pipe_name[PIPE_NAME_MAX];
    char response[MESSAGE_LEN];

    sprintf(self_pipe_name, "%s_RD", self);

    if ((dp = opendir(domain)) == NULL) {
        fprintf(stderr, "cannot open directory: %s\n", domain);
        return;
    }

    chdir(domain);

    while ((file = readdir(dp)) != NULL) {
        /*
         * Loop over the folder, find FIFOs ended with "_RD"
         */
        stat(file->d_name, &file_stat);
        if (!S_ISFIFO(file_stat.st_mode)) {
            continue;
        }
        int name_len = strlen(file->d_name);
        if (strcmp(&((file->d_name)[name_len - 3]), "_RD") != 0) {
            /*
             * Get the last three char and compare
             */
            continue;
        }

        if (strcmp(file->d_name, self_pipe_name) == 0) {
            /*
             * Check if st is self
             */
            continue;
        }
        /*
         * Get the termination byte
         */
        uint8_t ter_byte = message[SAYCONT_TER];

        if (ter_byte != TERMINATION) {
            ter_byte = 0;
        }

        /*
         * Create a response message
         */
        memset(response, 0, MESSAGE_LEN);
        strcpy(&response[TYPE_LEN], self);
        memcpy(&response[TYPE_LEN + PIPE_NAME_MAX],
               &message[TYPE_LEN], SAYCONT_MSG_LEN);

        response[0] = RECVCONT;
        response[SAYCONT_TER] = ter_byte;

        /*
         * Write the message, in blocking mode
         */
        p = open(file->d_name, O_WRONLY);
        if (p == -1) {
            return;
        }
        write(p, response, MESSAGE_LEN);
        close(p);

    }

    chdir("..");
    closedir(dp);
}

void handle_sig_usr1() {
    /*
     * Handle client handlers exited.
     */
    int status;
    pid_t pid;
    sleep(1);

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        /*
         * Loop over all child exited.
         */
        continue;
    }

    return;
}

int main(int argc, char **argv) {
    int p;
    char *gevent = "gevent";
    mkfifo(gevent, 0666);
    char message[MESSAGE_LEN];

    pid_t child;
    pid_t parent_pid;
    parent_pid = getpid();

    signal(SIGUSR1, handle_sig_usr1);
    while (1) {
        /*
         * Start global process
         */
        p = open(gevent, O_RDWR);
        if (p == -1) {
            return -1;
        }
        read(p, message, MESSAGE_LEN);
        close(p);

        uint16_t tcode = message[1] << 8 | message[0];

        if (tcode != CONNECT) {
            continue;
        }

        child = fork();
        if (child == -1) {
            printf("fork failed\n");
            exit(-1);
        }
        if (child) {
            /*
             * Parent: continue listening
             */
            continue;
        } else {
            /*
             * Child: break the loop.
             */
            break;
        }
    }

    /*
     * Client handler: calculate the name of pipe
     * Create the pipe for the client
     */

    char id[PIPE_NAME_MAX];
    char domain[PIPE_NAME_MAX];
    memcpy(id, &message[2], PIPE_NAME_MAX);
    memcpy(domain, &message[2 + PIPE_NAME_MAX], PIPE_NAME_MAX);

    char p_RD_name[PIPE_PATH_MAX];
    char p_WR_name[PIPE_PATH_MAX];
    sprintf(p_RD_name, "%s/%s_RD", domain, id);
    sprintf(p_WR_name, "%s/%s_WR", domain, id);

    char domain_path[PIPE_NAME_MAX + 1];
    sprintf(domain_path, "%s/", domain);

    struct stat st = {0};
    if (stat(domain_path, &st) == -1) {
        mkdir(domain_path, 0700);
    }
    mkfifo(p_RD_name, 0666);
    mkfifo(p_WR_name, 0666);


    /*
     * Client handler: listening in non-blocking mode
     */
    int p_wr;
    int p_rd;
    p_wr = open(p_WR_name, O_RDWR);
    if (p_wr == -1) {
        kill(parent_pid, SIGUSR1);
        return -1;
    }

    struct pollfd npipes[2];
    npipes[0].fd = p_wr;
    npipes[0].events = POLLIN;

    time_t last_ping;
    time_t last_pong;
    time_t now;

    time(&last_ping);
    time(&last_pong);

    while (1) {
        int ret = poll(npipes, NFDS, POLL_TIMEOUT);
        /*
         * reading in non-blocking mode
         */
        time(&now);

        if (ret == -1) {
            printf("poll error\n");
            exit(1);
        }


        if (npipes[0].revents & POLLIN) {
            /*
             * Reading pipe is ready
             */
            read(p_wr, message, MESSAGE_LEN);

            uint16_t tcode = message[1] << 8 | message[0];

            if (tcode == CONNECT) {
                continue;
            } else if (tcode == SAY) {
                say_handler(domain, id, message);
            } else if (tcode == SAYCONT) {
                saycont_handler(domain, id, message);
            } else if (tcode == PONG) {
                time(&last_pong);
                continue;
            } else if (tcode == DISCONNECT) {
                break;
            } else {
                continue;
            }
        }

        double ping_diff = difftime(now, last_ping);
        double pong_diff = difftime(now, last_pong);
        if (ping_diff >= 15) {
            /*
             * Send PING every 15 sec.
             */
            char response[MESSAGE_LEN];
            memset(response, 0, MESSAGE_LEN);
            response[0] = PING;
            p_rd = open(p_RD_name, O_RDWR);
            if (p_rd == -1) {
                return -1;
            }
            write(p_rd, response, MESSAGE_LEN);
            close(p_wr);
            time(&last_ping);
            continue;
        }
        if (pong_diff >= 15) {
            /*
             * If no PONG send back in 15 sec, quit.
             */
            break;
        }
    }
    close(p_rd);
    unlink(p_RD_name);
    unlink(p_WR_name);
    kill(parent_pid, SIGUSR1);

    return 0;
}
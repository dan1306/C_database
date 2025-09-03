#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <poll.h>
#include <stdbool.h>


#include "common.h"
#include "parse.h"
#include "srvpoll.h"
#include "file.h"

clientstate_t clientStates[MAX_CLIENTS] = {0};

void print_usage(char *argv[]) {
    printf("Usage: %s -n -f <database file> -p <port>\n", argv[0]);
    printf("\t-n create new database file\n");
    printf("\t-f (required) path to database file\n");
    printf("\t-p port number\n");
}

// Server poll loop
void poll_loop(unsigned short port, struct dbheader_t *dbhdr, struct employee_t *employees, int dbfd){
    int listen_fd, conn_fd, freeslot;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds = 1;

    init_clients(clientStates);

    // Create listening socket
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Allow rebind quickly
    int opt = 1;
    if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if(bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if(listen(listen_fd, 19) == -1){
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);

    memset(fds, 0, sizeof(fds));
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    nfds = 1;

    while(1) {
        int ii = 1;
        for(int i = 0; i < MAX_CLIENTS; i++){
            if(clientStates[i].fd != -1){
                fds[ii].fd  = clientStates[i].fd;
                fds[ii].events = POLLIN;
                ii++;
            }
        }
        nfds = ii;

        int n_events = poll(fds, nfds, -1);
        if(n_events == -1) {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        // New connection
        if(fds[0].revents & POLLIN) {
            if((conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len)) == -1) {
                perror("accept");
                continue;    
            }

            freeslot = find_free_slot(clientStates);
            printf("New client connected at slot %d fd %d\n", freeslot, conn_fd);
            if(freeslot == -1){
                printf("Server full: closing new connection\n");
                close(conn_fd);
            } else {
                clientStates[freeslot].fd = conn_fd;
                clientStates[freeslot].e_state = STATE_HELLO;
                memset(clientStates[freeslot].buffer, 0, sizeof(clientStates[freeslot].buffer));
            }
            n_events--;
        }

        // Client activity
        for(int i = 1; i <= nfds && n_events > 0; i++){
            if(fds[i].revents & POLLIN) {
                n_events--;
                int fd = fds[i].fd;
                int slot= find_slot_by_fd(clientStates, fd);
                if(slot == -1) {
                    printf("Could not find client slot for fd %d\n", fd);
                    continue;
                }
                ssize_t bytes_read = read(fd, &clientStates[slot].buffer, sizeof(dbproto_hdr_t));
                if(bytes_read <= 0 ){
                    close(fd);
                    clientStates[slot].fd = -1;
                    clientStates[slot].e_state = STATE_DISCONNECTED;
                    printf("Client disconnected\n");
                } else {
                    handle_client_fsm(dbhdr, &employees, &clientStates[slot], dbfd);
                }
            }
        }
    }
}

int main(int argc, char *argv[]){
    int c;
    bool newfile = false;
    char *filepath = NULL;
    unsigned short port = 0;

    int dbfd = -1;
    struct dbheader_t *dbhdr = NULL;
    struct employee_t *employees = NULL;

    while((c = getopt(argc, argv, "nf:p:")) != -1){
        switch(c){
            case 'n':
                newfile = true;
                break;
            case 'f':
                filepath = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            default:
                print_usage(argv);
                return -1;
        }
    }

    if(!filepath || port == 0){
        print_usage(argv);
        return -1;
    }

    if(newfile){
        dbfd = create_db_file(filepath);
        if(dbfd == STATUS_ERROR) return -1;
        create_db_header(dbfd, &dbhdr);
    } else {
        dbfd = open_db_file(filepath);
        if(dbfd == STATUS_ERROR) return -1;
        if(validate_db_header(dbfd, &dbhdr) == STATUS_ERROR){
            printf("Invalid database header\n");
            return -1;
        }
    }

    read_employees(dbfd, dbhdr, &employees);

    poll_loop(port, dbhdr, employees, dbfd);

    output_file(dbfd, dbhdr, employees);
    return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <sys/time.h>
#include <arpa/inet.h>
#import <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"
#include "file.h"
#include "parse.h"
#include "srvpoll.h"

#define MAX_CLIENTS 256

clientstate_t clientStates[MAX_CLIENTS] = {0};

void print_usage(char *argv[]) {
    printf("Usage: %s -n of <database file>\n", argv[0]);
    printf("\t -n create new database file\n");
    printf("\t -f (required) path to database file\n");
}

// new
void poll_loop(unsigned short port, stuct dbheader_t *dbhdr, struct employee_t *employees){
    int listen_fd, conn_fd, freeslot;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds = 1;
    int opt = 1;

    // Initialize client states
    init_clients(&clientStates);

    // Create listening socket
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Set up server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind
    if(bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Listen
    if(listen(listen_fd, 19) == -1){
        perror('listen');
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);

    memset(fd, 0, sizeof(fds));
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    nfds = 1;

    while(1) {
        int ii = 1;
        for(int i = 0; i < MAX_CLIENTS; i++){
            if(clientStates[i].fd != -1){
                fds[ii].fd  = clientStates[i].fd; // Offset by 1 for listen_fd
                fds[ii].events = POLLIN;
                ii++;
            }
        }

        // Wait for an event on one of the sockets
        int n_events = poll(fds, nfds, -1); // -1  means no timeout
        if(n_events == -1) {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        // Check for new connection
        if(fds[0].revents & POLLIN) {
            if((conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len)) == -1) {
                perror("accept");
                continue;    
            }
        
            printf("New connection from %s:%d\n",
            inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
            freeslot = find_free_slot(&clientStates);
            if(freeslot == -1){
                printf("Server full: closing new connection\n");
                close(conn_fd);
            } else {
                clientStates[freeslot].fs = conn_fd;
                clientStates[freeslot].state = STATE_CONNECTED;
                nfds++;
                printf("Slot %d has fd %d\n", freeslot, clientStates[freeslot].fd);
            }
            n_events--;
        }

        // check each clients for read/write activity
        for(int i = 1; i <= nfds && n_events > 0; i++){ // start from 1 to skip the listen_fd
            if(fds[i].revents & POLLIN) {
                n_events--;

                int fd = fds[i].fd;
                int slot= find_free_slot(&clientStates, fd);
                ssize_t bytes_read = read(fd, &clientStates[slot].buffer, sizeof(clientStates[slot].buffer));
                if(bytes_read <= 0 ){
                    // connection closed or error
                    close(fd);
                    if(slot == -1){
                        printf("Tried to close fd that doesn't exist?\n");
                    } else {
                        clientStates[slot].fd = -1; // Free up the slot
                        clientStates[slot].state = STATE_DISCONNECTED;
                        printf("Client disconnected or error");
                        nfds--;
                    }
                } else {
                    printf("Received data fromm the client: %s\n", clientStates[slot].buffer);
                    handle_client_fsm(dbhdr, employees, &clientStates[slot])
                }
            }
        }
    }

}

int main(int argc, char *argv[]){
    int c;
    bool newfile = false;
    char *portarg = NULL;
    unsigned short port = 0;
    char *filepath = NULL;
    char *addstring = NULL;
    bool list = false;

    int dbfd = -1;
    struct dbheader_t *dbhdr = NULL;
    struct employee_t *employees = NULL;
    while((c = getopt(argc, argv, "nf:p")) != -1){
        switch(c){
            case 'n':
                newfile = true;
                break;
            case 'f':
                filepath = optarg;
                break;
            // case 'a':
                // addstring = optarg;
                // break;
            case 'p':
                portarg = optarg;
                port = atoi(port);
                if(port == 0) {
                    printf("Bad port: %s\n", portarg);
                }
                break;
            // case 'l':
                // list = true;
                // break;
            case '?':
                printf("Unknown option -%c\n", c);
            default:
                return -1;   
        }

        // if(validate_db_header(dbfd, &dbhdr) == STATUS_ERROR){
        //     printf("Failed to validate database header one\n");
        //     return -1;
        // }
    }

    if(filepath == NULL){
        printf("Filepath is a required argument\n");
        print_usage(argv);
        return 0;
    }

    if(port == 0) {
        printf("Port not set\n");
        print_usage(argv);
        return 0;
    }

    if(newfile){
        dbfd = create_db_file(filepath);
        if (dbfd == STATUS_ERROR){
            printf("Unable to create database file\n");
            return STATUS_ERROR;
        }
        
        if (create_db_header(dbfd, &dbhdr) == STATUS_ERROR){
            printf("Failed to create database\n");
            return -1;
        }
    } else { 
        dbfd = open_db_file(filepath);
        if (dbfd == STATUS_ERROR){
            printf("Unable to open database filen\n");
            return -1;
        }

        if(validate_db_header(dbfd, &dbhdr) == STATUS_ERROR){
            printf("Failed to validate database header two\n");
            return -1;
        }
    }

    if(read_employees(dbfd, dbhdr, &employees) != STATUS_SUCCESS){
        printf("failed to read employees\n");
        return 0;
    }

    if(addstring){
        dbhdr->count++;
        employees = realloc(employees, dbhdr->count*(sizeof(struct employee_t)));
        add_employee(dbhdr, employees, addstring);
    }

    if(list){
        list_employees(dbhdr, employees);
    }

    poll_loop(port, dbhdr, employees);
    // printf("Newfile: %d\n", newfile);
    // printf("Filepath: %s\n", filepath);
    output_file(dbfd, dbhdr, employees);
    
    return 0;
}

// new
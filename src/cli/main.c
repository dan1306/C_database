#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>

#include "common.h"

#define BUF_SIZE 4096

// Send HELLO to server
int send_hello(int fd) {
    char buf[BUF_SIZE] = {0};
    dbproto_hdr_t *hdr = (dbproto_hdr_t *)buf;
    hdr->type = htons(MSG_HELLO_REQ);
    hdr->len = htons(1);

    dbproto_hello_req *hello = (dbproto_hello_req *)&hdr[1];
    hello->proto = htons(PROTO_VER);

    if(write(fd, buf, sizeof(dbproto_hdr_t) + sizeof(dbproto_hello_req)) <= 0) {
        perror("write");
        return STATUS_ERROR;
    }

    if(read(fd, buf, sizeof(dbproto_hdr_t) + sizeof(dbproto_hello_resp)) <= 0) {
        perror("read");
        return STATUS_ERROR;
    }

    hdr->type = ntohs(hdr->type);
    printf("DEBUG: Received HELLO response type %d\n", hdr->type);
    if(hdr->type != MSG_HELLO_RESP) {
        printf("Server HELLO failed.\n");
        return STATUS_ERROR;
    }

    printf("Connected to server, protocol OK.\n");
    return STATUS_SUCCESS;
}

// Send employee add request
int send_add(int fd, const char *emp_str) {
    char buf[BUF_SIZE] = {0};
    dbproto_hdr_t *hdr = (dbproto_hdr_t *)buf;
    hdr->type = htons(MSG_EMPLOYEE_ADD_REQ);
    hdr->len = htons(1);

    dbproto_employee_add_req *emp_req = (dbproto_employee_add_req *)&hdr[1];
    strncpy(emp_req->data, emp_str, sizeof(emp_req->data)-1);
    emp_req->data[sizeof(emp_req->data)-1] = '\0';

    if(write(fd, buf, sizeof(dbproto_hdr_t) + sizeof(dbproto_employee_add_req)) <= 0) {
        perror("write");
        return STATUS_ERROR;
    }

    if(read(fd, buf, sizeof(dbproto_hdr_t)) <= 0) {
        perror("read");
        return STATUS_ERROR;
    }

    hdr->type = ntohs(hdr->type);
    if(hdr->type != MSG_EMPLOYEE_ADD_RESP) {
        printf("Failed to add employee.\n");
        return STATUS_ERROR;
    }

    printf("Employee added successfully.\n");
    return STATUS_SUCCESS;
}

// Send list request
int send_list(int fd) {
    char buf[BUF_SIZE] = {0};
    dbproto_hdr_t *hdr = (dbproto_hdr_t *)buf;
    hdr->type = htons(MSG_EMPLOYEE_LIST_REQ);
    hdr->len = htons(0);

    if(write(fd, buf, sizeof(dbproto_hdr_t)) <= 0) {
        perror("write");
        return STATUS_ERROR;
    }

    if(read(fd, buf, sizeof(dbproto_hdr_t)) <= 0) {
        perror("read");
        return STATUS_ERROR;
    }

    hdr->type = ntohs(hdr->type);
    hdr->len = ntohs(hdr->len);

    if(hdr->type != MSG_EMPLOYEE_LIST_RESP) {
        printf("Failed to get employee list.\n");
        return STATUS_ERROR;
    }

    for(int i = 0; i < hdr->len; i++) {
        dbproto_employee_list_resp emp;
        if(read(fd, &emp, sizeof(dbproto_employee_list_resp)) <= 0) break;
        emp.hours = ntohl(emp.hours);
        printf("%s, %s, %u\n", emp.name, emp.address, emp.hours);
    }

    return STATUS_SUCCESS;
}

int main(int argc, char *argv[]) {
    char *host = NULL;
    unsigned short port = 0;
    char *addarg = NULL;
    bool list = false;

    int opt;
    while((opt = getopt(argc, argv, "h:p:a:l")) != -1) {
        switch(opt) {
            case 'h': host = optarg; break;
            case 'p': port = atoi(optarg); break;
            case 'a': addarg = optarg; break;
            case 'l': list = true; break;
            default:
                fprintf(stderr, "Usage: %s -h host -p port [-a addstring] [-l]\n", argv[0]);
                return -1;
        }
    }

    if(!host || port == 0) {
        fprintf(stderr, "Host and port required.\n");
        return -1;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) { perror("socket"); return -1; }

    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    if(inet_pton(AF_INET, host, &server.sin_addr) <= 0) {
        perror("inet_pton");
        close(fd);
        return -1;
    }

    if(connect(fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect");
        close(fd);
        return -1;
    }

    if(send_hello(fd) != STATUS_SUCCESS) {
        close(fd);
        return -1;
    }

    if(addarg) {
        if(send_add(fd, addarg) != STATUS_SUCCESS) {
            close(fd);
            return -1;
        }
    }
    if(list) {
        if(send_list(fd) != STATUS_SUCCESS) {
            close(fd);
            return -1;
        }
    }

    close(fd);
    return 0;
}


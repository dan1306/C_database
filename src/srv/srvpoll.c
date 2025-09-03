#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "srvpoll.h"
#include "parse.h"
#include "common.h"

// Reply with HELLO response
void fsm_reply_hello(clientstate_t *client, dbproto_hdr_t *hdr) {
    hdr->type = htons(MSG_HELLO_RESP);
    hdr->len = htons(1);

    dbproto_hello_resp *hello = (dbproto_hello_resp*)&hdr[1];
    hello->proto = htons(PROTO_VER);

    if(write(client->fd, hdr, sizeof(dbproto_hdr_t) + sizeof(dbproto_hello_resp)) == -1) {
        perror("write hello response");
    }
}

// Reply for employee ADD
void fsm_reply_add(clientstate_t *client, dbproto_hdr_t *hdr) {
    hdr->type = htons(MSG_EMPLOYEE_ADD_RESP);
    hdr->len = htons(0);
    if(write(client->fd, hdr, sizeof(dbproto_hdr_t)) == -1) {
        perror("write add response");
    }
}

// Reply with ERROR
void fsm_reply_hello_err(clientstate_t *client, dbproto_hdr_t *hdr) {
    hdr->type = htons(MSG_ERROR);
    hdr->len = htons(0);
    if(write(client->fd, hdr, sizeof(dbproto_hdr_t)) == -1) {
        perror("write error response");
    }
}

// Send employee list to client
void send_employees(struct dbheader_t *dbhdr, struct employee_t **employeeptr, clientstate_t *client) {
    dbproto_hdr_t *hdr = (dbproto_hdr_t*)client->buffer;
    hdr->type = htons(MSG_EMPLOYEE_LIST_RESP);
    hdr->len = htons(dbhdr->count);

    if(write(client->fd, hdr, sizeof(dbproto_hdr_t)) == -1) {
        perror("write list header");
        return;
    }

    struct employee_t *employees = *employeeptr;
    dbproto_employee_list_resp emp_resp;

    for (int i = 0; i < dbhdr->count; i++) {
        strncpy(emp_resp.name, employees[i].name, sizeof(emp_resp.name) - 1);
        emp_resp.name[sizeof(emp_resp.name) - 1] = '\0';
        strncpy(emp_resp.address, employees[i].address, sizeof(emp_resp.address) - 1);
        emp_resp.address[sizeof(emp_resp.address) - 1] = '\0';
        emp_resp.hours = htonl(employees[i].hours);
        if(write(client->fd, &emp_resp, sizeof(emp_resp)) == -1) {
            perror("write employee data");
            return;
        }
    }
}

// Handle client FSM
void handle_client_fsm(struct dbheader_t *dbhdr, struct employee_t **employeeptr, clientstate_t *client, int dbfd) {
    dbproto_hdr_t *hdr = (dbproto_hdr_t*)client->buffer;

    hdr->type = ntohs(hdr->type);
    hdr->len = ntohs(hdr->len);
    
    printf("DEBUG: Received message type %d, len %d, state %d\n", hdr->type, hdr->len, client->e_state);
    
    // Read additional payload if needed based on message type
    if (hdr->type == MSG_HELLO_REQ && hdr->len == 1) {
        ssize_t payload_read = read(client->fd, &client->buffer[sizeof(dbproto_hdr_t)], sizeof(dbproto_hello_req));
        if (payload_read <= 0) {
            printf("Failed to read HELLO payload\n");
            return;
        }
    } else if (hdr->type == MSG_EMPLOYEE_ADD_REQ && hdr->len == 1) {
        ssize_t payload_read = read(client->fd, &client->buffer[sizeof(dbproto_hdr_t)], sizeof(dbproto_employee_add_req));
        if (payload_read <= 0) {
            printf("Failed to read ADD payload\n");
            return;
        }
    }

    if (client->e_state == STATE_HELLO) {
        if (hdr->type != MSG_HELLO_REQ || hdr->len != 1) {
            fsm_reply_hello_err(client, hdr);
            return;
        }

        dbproto_hello_req *hello = (dbproto_hello_req*)&hdr[1];
        hello->proto = ntohs(hello->proto);
        if (hello->proto != PROTO_VER) {
            fsm_reply_hello_err(client, hdr);
            return;
        }

        fsm_reply_hello(client, hdr);
        client->e_state = STATE_MSG;
    }

    if (client->e_state == STATE_MSG) {
        if (hdr->type == MSG_EMPLOYEE_ADD_REQ) {
            dbproto_employee_add_req *employee_req = (dbproto_employee_add_req*)&hdr[1];
            if (add_employee(dbhdr, employeeptr, employee_req->data) != STATUS_SUCCESS) {
                fsm_reply_hello_err(client, hdr);
                return;
            } else {
                fsm_reply_add(client, hdr);
                output_file(dbfd, dbhdr, *employeeptr);
            }
        }

        if (hdr->type == MSG_EMPLOYEE_LIST_REQ) {
            send_employees(dbhdr, employeeptr, client);
        }
    }
}

// Initialize all client slots
void init_clients(clientstate_t* states) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        states[i].fd = -1;
        states[i].e_state = STATE_NEW;
        memset(states[i].buffer, 0, BUFF_SIZE);
    }
}

// Find free client slot
int find_free_slot(clientstate_t* states) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (states[i].fd == -1) return i;
    }
    return -1;
}

// Find client slot by fd
int find_slot_by_fd(clientstate_t* states, int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (states[i].fd == fd) return i;
    }
    return -1;
}


#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#define STATUS_ERROR  -1
#define STATUS_SUCCESS 0
#define PROTO_VER 100  // Protocol version

// Message types
typedef enum {
    MSG_HELLO_REQ,
    MSG_HELLO_RESP,
    MSG_EMPLOYEE_LIST_REQ,
    MSG_EMPLOYEE_LIST_RESP,
    MSG_EMPLOYEE_ADD_REQ,
    MSG_EMPLOYEE_ADD_RESP, 
    MSG_EMPLOYEE_DEL_REQ,
    MSG_EMPLOYEE_DEL_RESP,
    MSG_ERROR
} dbproto_type_e;

// Generic message header
typedef struct {
    dbproto_type_e type;
    uint16_t len; // length of message payload
} dbproto_hdr_t;

// Hello request / response
typedef struct {
    uint16_t proto; // protocol version
} dbproto_hello_req;

typedef struct {
    uint16_t proto; // protocol version
} dbproto_hello_resp;

// Employee add request
typedef struct {
    char data[256]; // employee info as string
} dbproto_employee_add_req;

// Employee list response
typedef struct {
    char name[250];
    char address[250];
    uint32_t hours; // network-safe 32-bit integer
} dbproto_employee_list_resp;

#endif


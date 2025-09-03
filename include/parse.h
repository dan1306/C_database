#ifndef PARSE_H
#define PARSE_H

#include <stdint.h>

#define HEADER_MAGIC 0x4f4f4144  // Magic number for DB validation

// Database header structure
struct dbheader_t {
    uint32_t magic;      // Magic number
    uint16_t version;    // DB version
    uint16_t count;      // Number of employees
    uint32_t filesize;   // Total file size
};

// Employee structure
struct employee_t {
    char name[250];      // Employee name
    char address[250];   // Employee address
    uint32_t hours;      // Hours worked
};

// Database file operations
// Returns 0 on success, -1 on error
int create_db_header(int fd, struct dbheader_t **headerOut);
int validate_db_header(int fd, struct dbheader_t **headerOut);
int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut);
void output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees);
int add_employee(struct dbheader_t *dbhdr, struct employee_t **employees, char *addstring);

// Utility
void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees);

#endif


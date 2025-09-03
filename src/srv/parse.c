#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "common.h"
#include "parse.h"

void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees){
   int i = 0;
   for(; i < dbhdr->count; i++){
      printf("Employee %d\n", i);
      printf("\tName: %s\n", employees[i].name);
      printf("\tAddress: %s\n", employees[i].address);
      printf("\tHours: %d\n", employees[i].hours);
   }
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t **employeeptr, char *addstring ){
   printf("DB currently has %d\n", dbhdr->count);

   // Make a copy since strtok modifies the input string
   char *input_copy = malloc(strlen(addstring) + 1);
   if(input_copy == NULL) {
      printf("Failed to allocate memory for input copy\n");
      return STATUS_ERROR;
   }
   strcpy(input_copy, addstring);

   char *name = strtok(input_copy, ",");
   if(name == NULL) {
      free(input_copy);
      return STATUS_ERROR;
   }

   char *addr = strtok(NULL, ",");
   if(addr == NULL){
      free(input_copy);
      return STATUS_ERROR;
   }

   char *hours = strtok(NULL, ",");
   if(hours == NULL) {
      free(input_copy);
      return STATUS_ERROR;
   }
   
   // Validate hours is a positive number
   int hours_val = atoi(hours);
   if(hours_val <= 0) {
      printf("Invalid hours value: %s\n", hours);
      free(input_copy);
      return STATUS_ERROR;
   }
   
   dbhdr->count++;
   struct employee_t *temp = realloc(*employeeptr, dbhdr->count*(sizeof(struct employee_t)));
   if(temp == NULL) {
      printf("Realloc failed\n");
      dbhdr->count--; // Restore original count
      free(input_copy);
      return STATUS_ERROR;
   }
   *employeeptr = temp;
   struct employee_t *employees = *employeeptr;

   strncpy(employees[dbhdr->count - 1].name, name, sizeof(employees[dbhdr->count -1].name) - 1);
   employees[dbhdr->count - 1].name[sizeof(employees[dbhdr->count -1].name) - 1] = '\0';
   strncpy(employees[dbhdr->count - 1].address, addr, sizeof(employees[dbhdr->count -1].address) - 1);
   employees[dbhdr->count - 1].address[sizeof(employees[dbhdr->count -1].address) - 1] = '\0';  

   employees[dbhdr->count - 1].hours = hours_val;
   free(input_copy);
   return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut ){
   if(fd < 0){
      printf("Got a bad FD from the user\n");
      return STATUS_ERROR;
   }
   int count = dbhdr->count;
   struct employee_t *employees = calloc(count, sizeof(struct employee_t) );
   if(employees == NULL){
      printf("Malloc failed\n");
      return STATUS_ERROR;
   }

   if(read(fd, employees, count*sizeof(struct employee_t)) != count*sizeof(struct employee_t)) {
      printf("Failed to read employee data\n");
      free(employees);
      return STATUS_ERROR;
   }

   int i = 0;
   for(; i < count; i++){
      employees[i].hours = ntohl(employees[i].hours);
   }
   *employeesOut = employees;
   return STATUS_SUCCESS;
}

void output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees){
   if(fd < 0){
      printf("Got a bad FD from the user");
      return;
   }
   // Ensures readability across systems with different endianness

   int realcount = dbhdr->count;

   dbhdr->magic = htonl(dbhdr->magic);
   dbhdr->filesize = htonl(sizeof(struct dbheader_t) + (sizeof(struct employee_t) * realcount));
   dbhdr->count = htons(dbhdr->count);
   dbhdr->version = htons(dbhdr->version);
   // Ensures readabilty across systems with different endianness
   // Ensures the file can be read correctly on machines with different 
   // architectures (e.g., Intel CPUs use little-endian, while some older 
   // systems use big-endian).
   if(lseek(fd, 0, SEEK_SET) == -1) {
      perror("lseek");
      return;
   }
   if(write(fd, dbhdr, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)) {
      perror("write header");
      return;
   }
   
   int i = 0;
   for(; i < realcount; i++){
      employees[i].hours = htonl(employees[i].hours);
      if(write(fd, &employees[i], sizeof(struct employee_t)) != sizeof(struct employee_t)) {
         perror("write employee");
         return;
      }
      employees[i].hours = ntohl(employees[i].hours);
   }

   dbhdr->magic = ntohl(dbhdr->magic);
   // dbhdr->filesize = ntohl(sizeof(struct dbheader_t) + struct(sizeof(employee_t)))
   dbhdr->filesize = ntohl(sizeof(struct dbheader_t) + sizeof(struct employee_t) * realcount);
   dbhdr->count = ntohs(dbhdr->count);
   dbhdr->version = ntohs(dbhdr->version);
   return;
}

int create_db_header(int fd, struct dbheader_t **headerOut){
   struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
   if(header == NULL) {
      printf("Malloc failed to create db header\n");
      return STATUS_ERROR;
   }
   
   header->version = 0x1;
   header->count = 0;
   header->magic = HEADER_MAGIC;
   header->filesize = sizeof(struct dbheader_t);

   *headerOut= header;

   return STATUS_SUCCESS;
}

int validate_db_header(int fd, struct dbheader_t **headerOut){
   if(fd < 0){
      printf("Got a bad FD from the user\n");
      return STATUS_ERROR;
   }
   struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
   if(header == NULL){
      printf("Malloc failed to create a db heder\n");
      return STATUS_ERROR;
   }

   if(read(fd, header, sizeof( struct dbheader_t)) != sizeof(struct dbheader_t)){
      perror("read");
      free(header);
      return STATUS_ERROR; 
   }
   header->version = ntohs(header->version);
   header->count =  ntohs(header->count);
   header->magic =  ntohl(header->magic);
   header->filesize =  ntohl(header->filesize);
   if(header->magic != HEADER_MAGIC){
      printf("Improper header magic\n");
      free(header);
      return -1;
   }

   if(header->version != 1){
      printf("Improper header version\n");
      free(header);
      return -1;
   }

   struct stat dbstat = {0};
   fstat(fd, &dbstat);
   if(header->filesize != dbstat.st_size){
      printf("Corrupted database\n");
      free(header);
      return -1;
   }

   *headerOut = header;
   return STATUS_SUCCESS;
}
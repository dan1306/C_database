#include <stdio.h>
#include <unistd.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "parse.h"


void output_file(int fd, struct dbheader_t *dbhdr){
   if(fd < 0){
      printf("Got a bad FD from the user");
      return;
   }
   dbhdr->magic = htonl(dbhdr->magic);
   dbhdr->filesize = htonl(dbhdr->filesize);
   dbhdr->count = htonl(dbhdr->count);
   dbhdr->version = htonl(dbhdr->version);

   lseek(fd, 0, SEEK_SET);
   write(fd, dbhdr, sizeof(struct dbheader_t));
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
   header->magic =  ntohs(header->magic);
   header->filesize =  ntohs(header->filesize);
   if(header->magic != HEADER_MAGIC){
      printf("Improper header version\n");
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
      printf("Corrupted databse\n");
      free(header);
      return -1;
   }

   *headerOut = header;
   return STATUS_SUCCESS;
}
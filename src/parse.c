#include <stdio.h>
#include <unistd.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "common.h"
#include "parse.h"

int create_db_header(int fd, struct dbheader_t **headerOut){
   struct dbheader_t header =  {0};
   header.version = 0x1;
   
}
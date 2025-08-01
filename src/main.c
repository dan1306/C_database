#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>

#include "common.h"
#include "file.h"
#include "parse.h"
void print_usage(char *argv[]) {
    printf("Usage: %s -n of <database file>\n", argv[0]);
    printf("\t -n create new database file\n");
    printf("\t -f (required) path to database file\n");
}
int main(int argc, char *argv[]){
    int c;
    bool newfile = false;
    char *filepath = NULL;

    int dbfd = -1;
    struct dbheader_t *dbhdr = NULL;
    while((c = getopt(argc, argv, "nf:")) != -1){
        switch(c){
            case 'n':
                newfile = true;
                break;
            case 'f':
                filepath = optarg;
                break;
            case '?':
                printf("Unknown option -%c\n", c);
            default:
                return -1;   
        }

        if(validate_db_header(dbfd, &dbhdr) == STATUS_ERROR){
            printf("Failed to validate database header\n");
            return -1;
        }
    }

    if(filepath == NULL){
        printf("Filepath is a required argument\n");
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
            printf("Failed to validate database heafer\n");
            return -1;
        }
    }
    printf("Newfile: %d\n", newfile);
    printf("Filepath: %s\n", filepath);
    output_file(dbfd, dbhdr);
    
    return 0;
}
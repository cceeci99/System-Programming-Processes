#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "work.h"
#include "utils.h"


#define BUFFER_SIZE 512               // **** buffer works for any BUFFER_SIZE  ****


void find_urls(int fd, urls_struct urls) {

    // char array: word is dynamic array which will store bytes read from file for untill a complete word is read, 
    // realloc will be called if there isn't enough memory, and after that the word will be checked for url and added.

    int word_size = 0;
    int word_capacity = 8;

    char *word = malloc(word_capacity*sizeof(char));
    memset(word, 0, word_capacity);

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    
    do {                                // read file with buffer of fixed size untill there are no more bytes left to read
        memset(buffer, 0, BUFFER_SIZE);
        bytes_read = read(fd, buffer, BUFFER_SIZE);

        // dynamic way to read from file with fixed size buffer, and to check if given words contains URL
        for (int i=0; i<BUFFER_SIZE; i++) {
            
            if (buffer[i] != ' ' && buffer[i] != '\n') {    // while chars read are not space nor newline append them to the word array
                
                if (word_size >= word_capacity) {
                    word_capacity = word_capacity*2;
                    word = realloc(word, word_capacity*sizeof(char));
                }

                word[word_size++] = buffer[i];
            }
            else {      // when space or newline is reached then a complete word is made and call function to add it to the urls
                
                if (strlen(word) == 0) {
                    continue;
                }
                
                char *url = is_url(word);
                if (url == NULL) {
                    continue;
                }

                char *location = extract_location(url);

                add_url_location(location, urls);

                free(location);

                // deallocate current word and allocate for new one
                free(word);

                word = malloc(word_capacity*sizeof(char));
                memset(word, 0, word_capacity);

                word_size = 0;
            }
        }
    } while(bytes_read == BUFFER_SIZE);

    free(word);
}


void write_output_file(urls_struct urls, char* file) {

    mode_t fdmode = S_IRUSR | S_IWUSR;
    
    char *temp = strdup(file); 

    // clean filename from path
    char *token = strtok(temp, "/");
    char *filename;

    while (token != NULL) {
        filename = token;
        token = strtok(NULL, "/");
    } 
    
    if (filename == NULL) {
        filename = temp;
    }

    char *dir = "./results/";                               // output files are stored in directory results
    char *out = ".out";

    size_t length = strlen(dir) + strlen(filename) + strlen(out) + 1;
    
    char *output_file = malloc(length*sizeof(char));        // build the output_file name : ./results/filename.out
    memset(output_file, 0, length);

    memcpy(output_file, dir, strlen(dir));
    strcat(output_file, filename);
    strcat(output_file, out);

    int fd = open(output_file, O_WRONLY | O_CREAT, fdmode);
    if (fd == -1) {
        perror_exit("open");
    }
    
    //  Write results οn file, each one in new line
    for (int i=0; i<urls->size; i++) {

        char str_num[10];       // max integer has 10 digits
        memset(str_num, 0, 10);
        sprintf(str_num, "%d", urls->array[i]->num_of_appearances);
 
        // length of location and of num_of_appearances
        int loc_len = strlen(urls->array[i]->location);
        int num_len = strlen(str_num);

        // include a space between location and appearances and new line at the end :  di.uoa.gr 3
        size_t buffer_size = loc_len + 1 + num_len + 1;

        char *buffer = malloc(buffer_size + 1);
        memset(buffer, 0, buffer_size);

        /* Construct byte by byte using memcpy the desired output which is: location num_of_appearances in each line*/
        memcpy(buffer, urls->array[i]->location, loc_len);
        buffer[loc_len] = ' ';

        memcpy(buffer + loc_len + 1, str_num, num_len);
        buffer[buffer_size-1] = '\n';
        buffer[buffer_size] = '\0';

        if (write(fd, buffer, buffer_size) == -1 && errno != EINTR) {
            perror_exit("write");
        }

        free(buffer);
    }

    free(temp);
    free(output_file);

    close(fd);
}
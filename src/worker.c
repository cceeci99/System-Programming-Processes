#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#include "work.h"
#include "utils.h"


#define LENGTH 100  // length used for char array file_name


// Catches SIGTERM signal from manager, and handles it by deleting all resources, and closing files.
void sigterm_handler(int signum);


// global variables for urls_struct and file descriptors(fifo & file) so they can be handled when programm is interrupted by SIGINT (ctrl^c)
urls_struct urls;
int fifo_fd, file_d;


int main(int argc, char* argv[]) {

    signal(SIGINT, SIG_IGN);              // ignore SIGINT signal
    
    // process will be terminated from manager by signal SIGTERM (if it is in working state) or SIGKILL (if it is in stopped state)
    struct sigaction sa;
    sa.sa_handler = sigterm_handler;
    sigaction(SIGTERM, &sa, NULL);

    char *fifo_name = argv[1];

    fifo_fd = open(fifo_name, O_RDONLY);                         // open fifo (named_pipe)
    if (fifo_fd == -1) {
        perror_exit("open");
    }

    while(1) {

        char file_name[LENGTH];
        memset(file_name, 0, LENGTH);
        
        size_t read_bytes = read(fifo_fd, &file_name, LENGTH);      // read from pipe the name of the file to work on
        if ((read_bytes == -1) && (errno != EINTR)) {
            perror_exit("read");
        }

        file_d = open(file_name, O_RDONLY);                         // open the file
        if (file_d == -1) {
            perror_exit("open");
        }
        
        urls = create_urls_struct();

        find_urls(file_d, urls);

        write_output_file(urls, file_name);
        
        delete_urls_struct(urls);  urls = NULL;    // delete urls after completing the work on one file, don't waste memory
        
        close(file_d);

        raise(SIGSTOP);                 // signal
    }
}


void sigterm_handler(int signum) {

    delete_urls_struct(urls);           // free memory resources

    close(file_d); close(fifo_fd);       // close file descriptors

    exit(EXIT_SUCCESS);
}
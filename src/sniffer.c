#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <dirent.h>

#include "queue.h"
#include "utils.h"

#define READ 0
#define WRITE 1

#define BUFFERSIZE 4096
#define LENGTH 100




/*---- A struct of pid with boolean stopped for each worker ---- */


struct p_info {
    pid_t pid;
    int stopped;
};

int pid_size = 0;
int pid_capacity = 8;

struct p_info *workers;     // dynamic array of struct p_info used to store all worker's info

int find_pid(pid_t p);      // just return the position in array of given pid.


/*--------------------------------------------------------------------------*/

queue workers_queue;        // FIFO queue with workers process's PID

int pipe_fd[2];             // unnamed pipe
int fifo_d;

pid_t listener_pid;         // global listener process's PID, can be killed in handler


// signal handlers
void sigchld_handler(int signum);
void sigint_handler(int signum);


void clean_dir_results();


int main(int argc, char* argv[]) {

    mkdir("./results/", 0777);      // make results and pipes directories IF they don't exist
    mkdir("./pipes/", 0777);

    clean_dir_results();        // calling this function, deletes all last written results files

    // create a unnamed pipe and check for error
    int return_status = pipe(pipe_fd); 
    if (return_status == -1) {
        perror_exit("pipe");
    }

    /*----- Check for correct path argument ./sniffer [-p path] -----*/
    char* path_to_listen;

    if (argc == 3) {

        if (strcmp(argv[1], "-p") != 0) {
            perror_exit("wrong input");
        }

        // check if directory exists
        struct stat sb;                 
        if (stat(argv[2], &sb) == 0 && S_ISDIR(sb.st_mode)) {
            path_to_listen = argv[2];
        }
        else {
            perror_exit("path");
        }
    }
    else if (argc == 1) {
        path_to_listen = ".";
    }
    else {
        perror_exit("wrong input");
    }
    /*--------------------------------------------------------------------*/

    // fork Listener Process
    pid_t pid = fork();

    if (pid == -1) {
        perror_exit("fork");
    }
    else if (pid == 0) {     // Listener Process
        
        signal(SIGINT, SIG_IGN);                        // ignore SIGINT signal and get terminated by parent

        close(pipe_fd[READ]);

        dup2(pipe_fd[WRITE], WRITE);                    // send stdout to the pipe
        close(pipe_fd[WRITE]);                          // pipe fd is not needed

        char* command = "inotifywait";
        char* args[] = {"inotifywait", "-m" , "-e", "create", "-e", "moved_to", path_to_listen, NULL};

        int status_code = execvp(command, args);        // execute command inotifywait

        if (status_code == -1) {
            perror_exit("execvp");
        }
    }
    else {                  // Manager Process

        // handling signals
        struct sigaction sa;
        sa.sa_handler = sigint_handler;
        sigaction(SIGINT, &sa, NULL);

        struct sigaction ssa;
        ssa.sa_handler = sigchld_handler;
        ssa.sa_flags = SA_RESTART;           // if a manager gets sigchld signal when reading or opening a file/fifo, restart system call
        sigaction(SIGCHLD, &ssa, NULL);

        workers_queue = create_queue();

        workers = malloc(pid_capacity*sizeof(struct p_info));
        if (workers == NULL) {
            perror_exit("malloc");
        }

        listener_pid = pid;

        close(pipe_fd[WRITE]);

        char buffer[BUFFERSIZE];

        while (1) {
            memset(buffer, 0, BUFFERSIZE);
            
            while(read(pipe_fd[READ], buffer, BUFFERSIZE)<=0);

            buffer[strcspn(buffer, "\n")] = 0;           // remove newline from buffer
            
            char* token = strtok(buffer, " ");          // now split buffer with " " , each output contains  DIR EVENT FILENAME

            int i = 0;
            char* token_array[3];

            while(token != NULL) {                          // tokenize the buffer and get the path and the filename
                token_array[i++] = token;
                token = strtok(NULL, " ");
            }

            char* path_string = token_array[0];
            char* file_string = token_array[2];

            char path_to_file[LENGTH];                          // concatenate path & filename
            memset(path_to_file, 0, LENGTH);

            memcpy(path_to_file, path_string, strlen(path_string));
            strcat(path_to_file, file_string);

            char fifo_name[LENGTH];
            memset(fifo_name, 0, LENGTH);

            if (empty(workers_queue)) {

                pid_t ppid = fork();                            // fork a worker
                
                if (ppid == -1) {
                    perror_exit("fork");
                }
                if (ppid == 0) {     

                    sprintf(fifo_name, "./pipes/fifo_%d", getpid());

                    char* args[] = {"worker", fifo_name, NULL};        
                    execv("./worker", args);
                }
                
                // each fifo is stored in directory ./pipes/ with name fifo_PID, where PID is process id of worker
                sprintf(fifo_name, "./pipes/fifo_%d", ppid);

                mkfifo(fifo_name, 0777);

                fifo_d = open(fifo_name, O_WRONLY);                // open pipe
                if (fifo_d == -1) {
                    perror_exit("open");
                }

                size_t write_bytes = write(fifo_d, path_to_file, strlen(path_to_file));     // write the path-to-file on the pipe
                if ((write_bytes == -1) && (errno != EINTR)) {
                    perror_exit("write");
                }
                
                close(fifo_d);

                // insert new forked worker's pid into dynamic array workers, resize if it's full
                if(pid_size >= pid_capacity) {
                    pid_capacity = pid_capacity*2;
                    
                    workers = realloc(workers, pid_capacity*sizeof(struct p_info));
                    if (workers == NULL) {
                        perror_exit("malloc");
                    }
                }

                workers[pid_size++].pid = ppid;
                workers[pid_size].stopped = 0;
            }
            else {

                pid_t pid = dequeue(workers_queue);

                sprintf(fifo_name, "./pipes/fifo_%d", pid);

                fifo_d = open(fifo_name, O_WRONLY);
                if (fifo_d == -1) {
                    perror_exit("open");
                }

                size_t write_bytes = write(fifo_d, path_to_file, strlen(path_to_file));
                if ((write_bytes == -1) && (errno != EINTR)) {
                    perror_exit("write");
                }

                close(fifo_d);

                int pos = find_pid(pid);
                workers[pos].stopped = 0;

                kill(pid, SIGCONT);
            }
        }
    }
}


void sigchld_handler(int signum) {

    int pid, status;

    while (1) {
        pid = waitpid (-1, &status, WNOHANG | WUNTRACED);

        if (pid == listener_pid) {                  // exclude listener pid
            continue;
        }

        if (pid > 0 && pid != listener_pid) {       // Listener PID shouldn't be in workers queue

            int pos = find_pid(pid);

            if (WIFSTOPPED(status)) {       // When Worker status changed from SIGSTOP signal

                if (!workers[pos].stopped) {    // safe code because enqueue method is O(1) and there is no problem with catching signals from many workers at the same time

                    enqueue(workers_queue, pid);
                    workers[pos].stopped = 1;

                    // UNCOMMENT TO SEE PRINT WHEN WORKER HAS FINISHED
                    // printf("Worker %d in WORKING state finished his work and stopped by signal:%d (SIGSTOP)\n", pid, WSTOPSIG(status));
                }
            }

            /* -----IGNORE THIS CODE, UNCOMMENT ONLY TO SEE PRINTS WHEN WORKER IS TERMINATED-----

            else if (WIFEXITED(status)) {    // worker status changed is state by sys call exit()
                if (workers[pos].stopped) {
                    printf("Worker %d in stopped state, terminated and exited with status=%d\n", pid, WEXITSTATUS(status));
                }
                else {
                    printf("Worker %d in working state, terminated and exited with status=%d\n", pid, WEXITSTATUS(status));
                }
            } */
        }
        else if (pid < 0 && errno != ECHILD) {
            perror ("waitpid");
            break;
        }
        else {
            break;
        }
    }
}


void sigint_handler(int signum) {
    
    kill(listener_pid, SIGKILL);

    close(fifo_d);      // close last fifo in case it hasn't closed

    for (int i=0; i<pid_size; i++) {

        if (workers[i].stopped) {
            // send SIGTERM which will get in pending state, and when child process is continued (SIGCONT), 
            // it will be delivered and terminate the child
            kill(workers[i].pid, SIGTERM);
            kill(workers[i].pid, SIGCONT);
        }
        else {
            // Workers who are not stopped will get SIGTERM signal from manager so they can handle and clean up resources
            kill(workers[i].pid, SIGTERM);
        }
       
        // remove opened named_pipe with each worker
        char fifo_name[LENGTH];
        memset(fifo_name, 0, LENGTH);
        sprintf(fifo_name, "./pipes/fifo_%d", workers[i].pid);

        unlink(fifo_name);
    }

    // free resources
    free(workers);
    delete_queue(workers_queue);

    close(pipe_fd[READ]);

    exit(EXIT_SUCCESS);
}


// function to clean last writtern results 
void clean_dir_results() {

    char *dir = "./results/";       // directory


    DIR *dir_ptr = opendir(dir);
    if (dir_ptr == NULL) {
        perror_exit("opendir");
    }

    struct dirent *ptr;
    while ((ptr = readdir(dir_ptr)) != NULL) {
        
        if (!strcmp(ptr->d_name, ".") || !strcmp(ptr->d_name, ".."))    // skip . and .. entries
            continue;

        char *path = malloc(strlen(dir) + strlen(ptr->d_name));
        strcpy(path, dir);
        strcat(path, ptr->d_name);

        if (unlink(path) == -1) {
            perror_exit("unlink");
        }

        free(path);
    }

    closedir(dir_ptr);
}


int find_pid(pid_t pid) {          // just return the position in the array of given pid

    for (int i=0; i<pid_size; i++) {
        if (workers[i].pid == pid) {
            return i;
        }
    }
}
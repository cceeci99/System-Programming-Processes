        |--------------   README:  System Programming, Project1, Semester: 6th, 2022  ------------------|
        |                                                                                               |
        |        Name:  TSVETOMIR IVANOV                                                                |
        |        sdi: 1115201900066                                                                     |
        |                                                                                               |
        |-----------------------------------------------------------------------------------------------|


-       -       -       -       -       -       -       -       -      -       -       -       -       -      -     


Project's Contents
=============================================================

pipes directory   -> where named_pipes are stored during the programm
results directory -> where .out files will be stored
* pipes & results directory are not given by default, but program generates them with mkdir() system call. mkdir creates directory only if they not exist *

src directory -> where source files are stored
build directory   -> where objects files are stored

- sniffer.c  -> main source file containing Manager & Listener Process
- worker.c   -> secondary main source file containing Worker Process

finder.sh  -> bash script 

Makefile
Readme

******** !!!! ********                                                                                                         ***
sniffer, worker executables & finder.sh's are strictly  in main's project (/project) directory alongside with Makefile, Readme ***
To work properly they must be not moved!!!                                                                                     ***
in build directory  are stored only object files  queue.o, urls.o utils.o, work.o (can be ignored)                             *** 
******** !!!! ********                                                                                                         ***


==============================================================


-       -       -       -       -       -       -       -       -       -       -       -       -       -       -


Compiling/Running Project
=========================================================================================================================================
- make worker sniffer -> command for seperate compilation of all sources files, and making the main executables sniffer & worker

*sniffer is the executable which user will run, and worker is the executable which is called from sniffer with execvp("./worker")*


Run sniffer:
-------------
- (1st way)  ./sniffer [-p path]   -> user run manually program giving optionally a path to monitor
- (2nd way)  make run              -> runs ./sniffer with default monitor path (current directory)


Run finder.sh:
--------------
./finder.sh TLD1 [TLD2 ...]  (give at least one , and optionally more TLDs)


Clean objects files/executables:
-------------------------------
- make clean


-       -       -       -       -       -       -       -       -       -       -       -       -       -       -


Code Explained
===========================================================================================================

Main Executables:  sniffer, worker  (must be in Projects's main Directory so the programm can run properly)

Source/Headers Files:
----------------------
- sniffer.c:  Contains the implementation of Manager Process alongside with Listener Process
- worker.c: Contains the implementation of Worker Process

- utils.c, utils.h: A simple utility function that is used by all other executables
- queue.c, queue.h: Contains an FIFO queue implementation that is used by Manager Process to insert workers's PID into the queue when they have finished their work
- urls.c, urls.h: Contains an implementation of struct urls, which will be used to store URLs's locations and their number of appearances, which will be written to the .out file
- work.c, work.h: Contains the methods that a Worker Process is using


Describing Entities of program, Implementation and their Functionality:
-----------------------------------------------------------------------

*** The structs and methods that are used are described in the headers and sources files ***

-  Listener Process: (sniffer.c)
--------------------------------
1. Listener process, made by Manager at the beginning, it calls with execvp() the inotifywait command with specified options, to monitor only MOVED_TO and CREATE events.
Also it gives inotifywait as argument the path on which it will monitor changes, which is given when running the executable ./sniffer [-p path]. The arguments are checked for errors.

2. Listener Process, is running inotifywait "forever". When the running programm is terminated from user by CTRL^C, parent process (Manager) must kill all child processes,
and so the Listener process -ignores- the SIGINT signal (or it will be terminated default by SIGINT).
When Manager catches SIGINT signal with -sigint_handler-, he sends signal SIGKILL signal to terminate forcefully the Listener Process.

*** Because inotifywait is specific command(an exectubale) that it's not handled from the programer , possible leaks can be shown in the listener process heap summary, but 
since it's killed forcefully, maybe the heap summary wont be shown because inotifywait doesnt terminate properly ***

- Worker Process:  (worker.c)
-----------------------------
1. Worker Process has it's own executable file <worker> (src: worker.c). It is called by manager with exev() when a new worker is spawned.

2. Worker's job is to wait in a loop untill a file's name is written to the named_pipe from manager. When new file is sent, worker opens it, and calls 
the appropriate methods to do the job (create_urls_struct -> find_urls -> write_output_file).

3. Worker has a -sigterm_handler- which is used to catch the SIGTERM signal sent by manager, to clean up resources and terminate properly
with exit status 0.

*** Because worker's executable, calls functions from other source file, it's normal that some local allocated variables used in those functions can't be 
deallocated in the sigterm_handler, because it's not best practice to use global variables. The only main resources which worker need to clean are the file descriptors
and the urls_struct. For that reason those are declared global in the worker.c src and they are freed in the handler. 
Possible leaks maybe -still reachable- could be shown in the heap summary of each worker's process ***

- Manager Process:  (sniffer.c)
-------------------------------

1. Manager Process waits in a loop untill a new event is sent by listener. When new event occurs, the manager reads from listener the output and stores 
it in buffer of fixed size (because for the purpose of the exercise, a fixed buffer of 4096 bytes will be enough to test plenty of events...).

2. After event is stored in buffer, it's tokenized to get the path and the filename. If the workers_queue is empty then manager
spawns a new worker, creates a fifo calls worker with exec passing him the fifo_name and writes on pipe the path-to-file.

3. If there is available worker, its poped from the queue and worker is continued after manager writes to pipe.

4. When worker finishes his work, a SIGCHLD signal is sent to manager process and proper handler is triggered. Manager waits until the child
changes its status by returning its pid, and push the pid to the queue.

*Flags  WNOHANG | WUNTRACED are used to prevent waitpid from getting in deadlock (blocking manager). 
WUNTRADED allows manager also to be return from waitpid if a child gets stopped as well as exiting or being killed.


Explanation of Handling Signals, Managing Child Processes:
-------------------------------------------------------------

- Manager must kill all processes in case user insert CTRL^C and SIGINT signal is sent to the manager.

- To achieve that, Manager keeps all child processes's (listener & workers) pid. Listener PID is kept in global variable
and can be signaled in sigint handler with signal=9 (SIGKILL).

- For workers, manager keeps an array of  -struct p_info {pid_t pid, int stopped}- 
to know which workers are stopped.

- When a worker is not stopped and SIGINT is sent to manager, worker ignores SIGINT, and is terminated from manager
by signal SIGTERM, which handles it with -sigterm_handler- to free resources.

- When a worker is stopped (meaning that he has sent SIGSTOP to him), no more signals except SIGKILL (& SIGCONT) can be delivered.
Manager sends SIGTERM signal, and that signal gets in pending state, meaning that it will be delivered to child process when its continued. After SIGTERM, manager
sends SIGCONT, and then the child catches that signal and terminates properly by exiting with status 1.

- When worker catches SIGTERM and exits with status 0, a SIGCHLD signal is sent again to manager process and is handled with the same way.
The only difference is that either stopped or not Worker is not pushed to the queue. There are optional printf commands to see the Functionality
of handling signals, that can be uncommented by user.

** Because system level calls are usually fast, in the case of our exercise, all workers finish their work instantly, and 
interrupting a worker who is not stopped may not occur. But if occurs, undefined behaviour is possible in that case as we don't know at which point
a worker will be interrupted and if there will be some corrupted output file or not. But properly handling of signals is done in the code **

** During the testing, a lots of files were moved (around 15) and program ran without problem. Also with valgrind possible leaks were not found. But sometimes undefined behaviour may occur if 
running with valgrind because it takes time to write output from valgrrind to terminal.**
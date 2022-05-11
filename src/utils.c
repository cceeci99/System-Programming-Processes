#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils.h"


void perror_exit(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}
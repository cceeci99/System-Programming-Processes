#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "urls.h"
#include "utils.h"


#define INITIAL_ARRAY_SIZE 8                // initial size for the url array, after it's filled it is reallocated 


urls_struct create_urls_struct() {

    // Allocate Memory for urls_struct
    urls_struct urls = malloc(sizeof(urls_struct));
    if (urls == NULL) {
        perror_exit("malloc");
    }

    urls->capacity = INITIAL_ARRAY_SIZE;
    urls->size = 0;

    urls->array = malloc(urls->capacity * sizeof(url_struct *));
    if (urls->array == NULL) {
        perror_exit("malloc");
    }

    for (int i=0; i<urls->capacity; i++) {
        urls->array[i] = malloc(sizeof(url_struct));
        if (urls->array[i] == NULL) {
            perror_exit("malloc");
        }

        urls->array[i]->location = NULL;
        urls->array[i]->num_of_appearances = 0;
    }

    return urls;
}


void delete_urls_struct(urls_struct urls) {
    
    if (urls == NULL) {
        return;
    }

    // free resources
    for (int i=0; i<urls->capacity; i++) {
        free(urls->array[i]->location);
        free(urls->array[i]);
    }

    free(urls->array);
    free(urls);
}


char* is_url(char *word) {

    return strstr(word, "http://");
}


char* extract_location(char *url) {

    char *str;
    char *location_t;

    // search for www. and extract only location without it e.x http://www.di.uoa.gr/staff  -> di.uoa.gr/staff
    if (str = strstr(url, "www.")) {
        location_t = strdup(&str[0] + 4);                
    }
    else {
        str = strstr(url, "/");
        if (str != NULL) {
            location_t = strdup(&str[0] + 2);     
        }
    }

    // filter all locations by removing any paths e.x  http://www.di.uoa.gr/Odhgos_Spoudwn/Praktiki ->  only di.uoa.gr
    str = strstr(location_t, "/");
    if (str != NULL) {
        size_t len = str - location_t;

        char *str2 = malloc(len+1);
        if (str2 != NULL) {
            memcpy(str2, location_t, len);
            str2[len] = 0;

            memset(location_t, 0, strlen(location_t));
            memcpy(location_t, str2, len);
        }
        free(str2);
    }

    // filter location from ports e.x http://www.site.org:80/ -> only site.org
    str = strstr(location_t, ":");
    if (str != NULL) {
        size_t len = str - location_t;

        char *str2 = malloc(len+1);
        if (str2 != NULL) {
            memcpy(str2, location_t, len);
            str2[len] = 0;

            memset(location_t, 0, strlen(location_t));
            memcpy(location_t, str2, len);
        }
        free(str2);
    }

    return location_t;
}


void add_url_location(char* location_t, urls_struct urls) {
    
    // check if location is already in array
    for (int i=0; i<urls->size; i++) {
        if (strcmp(urls->array[i]->location, location_t) == 0) {

            urls->array[i]->num_of_appearances++;
            return;
        }
    }

    if (urls->size >= urls->capacity) {     // Resize
        urls->capacity = urls->capacity*2;

        urls->array = realloc(urls->array, urls->capacity*sizeof(url_struct *));
        if (urls->array == NULL) {
            perror_exit("realloc");
        }

        for (int i = urls->size; i < urls->capacity; i++) {
            urls->array[i] = malloc(sizeof(url_struct));
            if (urls->array[i] == NULL) {
                perror_exit("malloc");
            }
            urls->array[i]->location = NULL;
            urls->array[i]->num_of_appearances = 0;
        }
    }

    // Add url's location
    urls->array[urls->size]->location = strdup(location_t);
    urls->array[urls->size]->num_of_appearances++;
    urls->size++;
}
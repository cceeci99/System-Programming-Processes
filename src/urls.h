#ifndef URLS_H
#define URLS_H


// -------------- Structs Used to store information about URL's location ----------------------- //


/* An url struct stores its location string and number of appearances */
struct url {
    char* location;
    int num_of_appearances;
};

typedef struct url* url_struct;         // pointer to url


/* An urls struct stores url array, size and capacity */
struct urls {
    url_struct *array;      

    int size;
    int capacity;
};

typedef struct urls* urls_struct;       // pointer to urls_struct


// -------------- Methods Used ---------------------------- //


/*---- Create new urls_struct and allocate memory ----*/
urls_struct create_urls_struct();


/*---- Delete allocated urls_struct ----*/
void delete_urls_struct(urls_struct urls);


/*---- Check if given word contains url by returning it, NULL if not ----*/
char* is_url(char *word);


/* ---- Exctract Location from given URL ---- */
char* extract_location(char *url);


/* ---- Add 'url' in urls_struct  ---- */
void add_url_location(char *location_t, urls_struct);


#endif //URLS_H
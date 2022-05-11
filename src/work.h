#ifndef WORK_H
#define WORK_H

#include "urls.h"


/*---- Find urls from given file ----*/
void find_urls(int file_d, urls_struct urls);


/*---- Write urls on output file with name file.out ----*/
void write_output_file(urls_struct urls, char* file);


#endif // WORK_H
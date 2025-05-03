#ifndef UTILS_H
#define UTILS_H

#include "http.h"

int compress_string(const char *input, char **output, int *output_len);
char *header_as_string(Header* headers,int headers_count);
char *str_slice(char *str,int start,int end);
char **tokenizeString(char *string,char *delimiter, int *counts);
void printff(const char *format, ...);

#endif

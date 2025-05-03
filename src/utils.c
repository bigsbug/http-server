#include <zlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "http.h"


char *header_as_string(Header* headers,int headers_count){
	char *crlf = "\r\n";
	int crlf_length = 2;

	char *header_start = ": ";
	int header_start_length = 2;

	int all_headers_size=0;

	for(int i=0;i<headers_count;i++){
		all_headers_size += strlen(headers[i].key) + strlen(headers[i].key) + crlf_length + header_start_length;
	};

	char buf[all_headers_size];
	char *all_headers = malloc(sizeof(char)+ all_headers_size +1);
	all_headers[0] = '\0'; // make it valid empty string

	for(int i=0;i<headers_count;i++){
		strncat(all_headers,headers[i].key,sizeof(buf));
		strncat(all_headers,header_start,sizeof(buf));
		strncat(all_headers,headers[i].value,sizeof(buf));
		strncat(all_headers,crlf,sizeof(buf));
	};

	return all_headers;
}

char *str_slice(char *str,int start,int end){
	int length = end-start;
	char *buffer = malloc(length+1);
	for(int i = 0; i < length ;i++){
		buffer[i] = str[start + i];
	};
	buffer[length]='\0';
	return buffer;
};

char **tokenizeString(char *string,char *delimiter, int *counts){
	*counts = 0;
	int string_length = strlen(string);
	int delimiter_length = strlen(delimiter);
	int token_counter = 0;

	if(string_length - delimiter_length < 0){
		char **tokens = malloc(sizeof(char*));
		tokens[0] = strdup(string);
		return tokens;
	}

	// find total tokens to generate an array with match their size
	for(int i=0;i<string_length - delimiter_length ;i++){
		char *slice = str_slice(string,i,i+delimiter_length);
		if( strcmp(slice,delimiter) == 0){
			(*counts)++;
		}
		free(slice);
	};

	// extra remained string slice at end
	(*counts)++;

	char **tokens = malloc(sizeof(char *) * (*counts));

	int last_token_pos = 0;
	// Save token as array
	for(int i=0;i<=string_length - delimiter_length ;i++){
		char *slice = str_slice(string,i,i+delimiter_length);
		if( strcmp(slice,delimiter) == 0){
			tokens[token_counter] = str_slice(string,last_token_pos,i);
			token_counter++;
			last_token_pos=i+delimiter_length;
		}
		free(slice);

		// +1 last remained string slice
		if( *counts  == token_counter + 1){
			tokens[token_counter] = str_slice(string,last_token_pos,string_length);
			token_counter++;
		};
	};
	return tokens;
}



// Function to compress a string using gzip
int compress_string(const char *input, char **output, int *output_len) {
    z_stream stream = {0};
    int ret;

    // Initialize zlib stream
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    // Initialize gzip compression (15 + 16 for gzip format)
    ret = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) {
        fprintf(stderr, "deflateInit2 failed: %d\n", ret);
        return ret;
    }

    // Set input data
    stream.avail_in = strlen(input) ;
    stream.next_in = (Bytef *)input;

    // Allocate memory for output
    *output_len = (int)deflateBound(&stream, stream.avail_in);
    *output = (char *)malloc(*output_len);
    if (*output == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        deflateEnd(&stream);
        return Z_MEM_ERROR;
    }

    // Set output buffer
    stream.avail_out = *output_len;
    stream.next_out = *output;

    // Perform compression
    ret = deflate(&stream, Z_FINISH);
    if (ret != Z_STREAM_END) {
        fprintf(stderr, "deflate failed: %d\n", ret);
        free(*output);
        *output = NULL;
        deflateEnd(&stream);
        return ret;
    }

    // Update output length
    *output_len = (int)stream.total_out;

    // Clean up
    deflateEnd(&stream);
    return Z_OK;
}

// int compress_string(const char *input, char **output, int *output_len) {
// return 0;}


void printff(const char *format, ...) {
    char buffer[2048];

    // Format the string like printf would
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    for (char *p = buffer; *p; ++p) {
        switch (*p) {
            case '\n': printf("\\n"); break;
            case '\t': printf("\\t"); break;
            case '\r': printf("\\r"); break;
            case '\b': printf("\\b"); break;
            case '\f': printf("\\f"); break;
            case '\v': printf("\\v"); break;
            case '\\': printf("\\\\"); break;
            case '\"': printf("\\\""); break;
            default:
                // For non-printables, show hex (optional)
                if ((unsigned char)*p < 32 || (unsigned char)*p > 126) {
                    printf("\\x%02x", (unsigned char)*p);
                } else {
                    putchar(*p);
                }
        }
    }
    printf("\n");
}

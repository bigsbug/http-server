#include <zlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>




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

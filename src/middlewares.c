
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"
#include "http.h"

// Middlewares
void middleware_add_content_length(HttpResponse *response,HttpRequest *request){
	
	Header *new_headers = malloc(sizeof(Header) *( response->headers_count + 1));
	for(int i=0;i < response->headers_count;i++){
		new_headers[i].key =  strdup(response->headers[i].key);
		new_headers[i].value =  strdup(response->headers[i].value);
	}	
	free(response->headers);

	char content_length_header[16];
	snprintf(content_length_header,sizeof(content_length_header),"%d",response->body_length);
	new_headers[ response->headers_count] = (Header){.key=strdup("Content-Length"),.value=strdup(content_length_header)};

	response->headers_count = response->headers_count + 1;
	response->headers = new_headers;
}

void middleware_add_encoding(HttpResponse *response,HttpRequest *request){
	char **encoding_types;
	int encoding_types_count;

	char *supported_encoding ="gzip";
	char *accepted_encoding = NULL;

	for(int i=0;i<request->headers_count;i++){
		if(strcmp(request->headers[i].key, "Accept-Encoding") != 0){ continue;}

		encoding_types  = tokenizeString(request->headers[i].value,", ",&encoding_types_count);
		for(int x=0;x<encoding_types_count;x++){
			printf("ENCODING: %s\n",encoding_types[x]);
			if(strcmp(encoding_types[x],supported_encoding) == 0){
				accepted_encoding = strdup(encoding_types[x]);
				break;
			};
		};

		free(encoding_types);
		break;

	};

	// Encoding Not Found
	if(accepted_encoding == NULL){
		return;
	}


	
	Header *new_headers = malloc(sizeof(Header) *( response->headers_count + 1));
	for(int i=0;i < response->headers_count;i++){
		new_headers[i].key =  strdup(response->headers[i].key);
		new_headers[i].value =  strdup(response->headers[i].value);
	}	
	free(response->headers);

	new_headers[ response->headers_count] = (Header){
		.key=strdup("Content-Encoding"),
		.value=strdup(accepted_encoding)
	};
	char *compressed_data = NULL;

    // Compress the string
    int ret = compress_string(response->body, &compressed_data, &response->body_length);

	response->headers_count = response->headers_count + 1;
	response->headers = new_headers;
	response->body = compressed_data;

	
}

void middleware_add_connection_status(HttpResponse *response,HttpRequest *request){
	int has_connectin_header = 0;
	for(int i=0;i<request->headers_count;i++){
		if(strcmp(request->headers[i].key, "Connection") == 0)
		{
			if(strcmp(request->headers[i].value, "close") == 0){
				has_connectin_header = 1;		
			}
			break;

		}
	};
	if(has_connectin_header ==0 ){
		printf("REJECT CLOSE CONNECTION\n");
		return;
	}
	printf(" NOT REJECT CLOSE CONNECTION\n");


	Header *new_headers = malloc(sizeof(Header) *( response->headers_count + 1));
	for(int i=0;i < response->headers_count;i++){
		new_headers[i].key =  strdup(response->headers[i].key);
		new_headers[i].value =  strdup(response->headers[i].value);
	}	
	free(response->headers);

	new_headers[ response->headers_count] = (Header){
		.key=strdup("Connection"),
		.value=strdup("close")
	};
	response->headers_count = response->headers_count + 1;
	response->headers = new_headers;
}

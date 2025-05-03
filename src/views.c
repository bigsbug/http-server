#include "http.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// VIEWS
HttpResponse *index_view(HttpRequest request){
	char header[1024];
	char *body = "";
	int body_size = strlen(body);

	int headers_count = 1;
	Header *headers = malloc(sizeof(Header)* headers_count);
	headers[0] = (Header){.key="Content-Type","text/plain"};

	HttpResponse *response = malloc(sizeof(HttpResponse));
	response->status="200 OK";
	response->headers=headers;
	response->headers_count=headers_count;
	response->body=body;
	return response;
};

HttpResponse *echo_view(HttpRequest request){
	char *body = request.arguments[0];
	int body_size = strlen(body);	

	char content_length_header[16];
	snprintf(content_length_header,sizeof(content_length_header),"%d",body_size);
	
	int headers_count = 1;
	Header *headers = malloc(sizeof(Header)* headers_count);
	headers[0] = (Header){.key="Content-Type","text/plain"};
	
	HttpResponse *response = malloc(sizeof(HttpResponse));
	response->status="200 OK";
	response->headers=headers;
	response->headers_count=headers_count;
	response->body=body;
	response->body_length=body_size;
	return response;
};

HttpResponse *user_agent_view(HttpRequest request){
	char header[1024];
	char *body = "";
	
	for(int i=0;i<request.headers_count;i++){
		if( strcmp("User-Agent",request.headers[i].key) == 0){
			body = request.headers[i].value;
			break;
		}
	}

	int body_size = strlen(body);

	HttpResponse *response = malloc(sizeof(HttpResponse));
	int headers_count = 1;
	response->headers = malloc(sizeof(Header)* headers_count);
	response->headers[0] = (Header){.key=strdup("Content-Type"),.value=strdup("text/plain")};
	response->headers_count=headers_count;


	response->status="200 OK";
	response->body = body;
	response->body_length=body_size;

	return response;
};

HttpResponse *files_view(HttpRequest request){
	printf("GET FILE\n");

	char header[1024];
	char *fileName = request.arguments[0];
	char *fileBasePath = "/tmp/data/codecrafters.io/http-server-tester/";
	int fileFullPathLength = strlen(fileName)+strlen(fileBasePath);
	char fileFullPath[fileFullPathLength +1];

	snprintf(fileFullPath,sizeof(fileFullPath),"%s%s",fileBasePath,fileName);
	fileFullPath[fileFullPathLength] = '\0'; 
	printf("File: [%s]",fileFullPath);
	FILE *file =fopen(fileFullPath,"r");
	long body_size = 0;
	char *body = NULL;

	if(file != NULL){

		// findout the file length
		fseek(file,0,SEEK_END);
		body_size = ftell(file);
		fseek(file,0,SEEK_SET);

		// store file content intro string
		body = malloc(body_size + 1);
		fread(body,1,body_size,file);
		// body[body_size] = '\0';
		fclose(file);

	}
	else{
		body = strdup("");
		perror("fopen failed");
	};
	

	int headers_count = 1;
	Header *headers = malloc(sizeof(Header)* headers_count);
	headers[0] = (Header){.key="Content-Type","application/octet-stream"};

	

	HttpResponse *response = malloc(sizeof(HttpResponse));
	response->status=(file != NULL) ? "200 OK" : "404 Not Found"; 
	response->headers=headers;
	response->headers_count=headers_count;
	response->body=body;
	response->body_length=body_size;

	return response;
};

HttpResponse *post_files_view(HttpRequest request){
	char header[1024];
	char *fileName = request.arguments[0];
	char *fileBasePath = "/tmp/data/codecrafters.io/http-server-tester/";
	int fileFullPathLength = strlen(fileName)+strlen(fileBasePath);
	char fileFullPath[fileFullPathLength +1];
	printf("Content: [%s]\n",request.body);
	snprintf(fileFullPath,sizeof(fileFullPath),"%s%s",fileBasePath,fileName);
	fileFullPath[fileFullPathLength] = '\0'; 
	printf("File: [%s]",fileFullPath);
	FILE *file =fopen(fileFullPath,"w");
	long body_size = strlen(request.body);

	if(file != NULL){
		fwrite(request.body,1,body_size,file);
		fclose(file);
	}
	else{
		perror("fopen failed");
	};
	

	int headers_count = 1;
	Header *headers = malloc(sizeof(Header)* headers_count);
	headers[0] = (Header){.key="Content-Type","application/octet-stream"};


	HttpResponse *response = malloc(sizeof(HttpResponse));
	response->status=(file != NULL) ? "201 Created" : "404 Not Found"; 
	response->headers=headers;
	response->headers_count=headers_count;
	response->body=request.body;
	response->body_length=body_size;

	return response;
};

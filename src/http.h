#ifndef HTTP_H

#define HTTP_H


typedef struct  {
	char *key;
	char *value;
} Header;

typedef struct{
	char *version;
	char *method;
	char *url;
	Header *headers;
	int headers_count;
	char *body;
	char **arguments;
	int arguments_count;
} HttpRequest;

typedef struct {
	char* status;
	Header* headers;
	int headers_count;
	char* body;
	int body_length;
} HttpResponse;

typedef struct {
	char *path;
	char *method;
	char **tokens;
	int tokens_count;
	HttpResponse *(*view)(HttpRequest request);
} Url;

#endif // MACRO

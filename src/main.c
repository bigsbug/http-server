#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <pthread.h>


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
	char* headers;
	char* body;
} HttpResponse;

typedef struct {
	char *path;
	char **tokens;
	int tokens_count;
	HttpResponse *(*view)(HttpRequest request);
} Url;
struct MatchedUrl {
	int index;
	int arguments_count;
	char **arguments;
};

struct ProcessBlock {
	int client;
	Url *urls;
	int urls_count;
};

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

char* make_response(char* status,char* headers,char* body){
	// Avoid Null inputs
	if( !status){
		return NULL;
	}
	char *http_version = "HTTP/1.1";
	char *crlf = "\r\n";
	// we had a padding after our version
	int http_version_len = strlen(http_version) + 1;
	// we had two crlf on our response
	int crlf_len = strlen(crlf)*2;
	int status_len = strlen(status);
	int header_len = strlen(headers);
	int body_len = strlen(body);
	int nullTerminator_len = 2;
	int total_len = http_version_len + crlf_len + status_len + header_len + body_len + nullTerminator_len;
	
	char *response = malloc(total_len);
	// malloc memory failed
	if (!response){
		return NULL;
	};

	int total_wrote = snprintf(response,total_len,"%s %s%s%s%s%s",http_version,status,crlf,headers,crlf,body);
	// check response is overflow
	if (total_wrote == 0 || total_wrote >= total_len){
		return NULL;
	};

	return response;
};

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

struct MatchedUrl match_url(char* dest_url,Url urls[],int urls_count){

	char **dest_url_tokens;
	int url_tokens_count;
	struct MatchedUrl best_url = {.index=-1,.arguments=NULL,.arguments_count=0} ;

	for(int i=0;i<urls_count;i++){
		int dest_url_tokens_count;
		dest_url_tokens = tokenizeString(dest_url,"/",&dest_url_tokens_count);

		// save corrsponding tokens that match with a '?' character
		char **dynamic_tokens = malloc(sizeof(char *) * urls[i].tokens_count);
		int dynamic_tokens_count = 0;

		if(dest_url_tokens_count != urls[i].tokens_count){
			continue;
		}

		for(int t=0;t < dest_url_tokens_count;t++){
			// printf("DEBUG:#3 URL Token [%s] %d\n",urls[i].tokens[t],dest_url_tokens_count);

			if(strcmp(urls[i].tokens[t] , "?")==0){
				best_url.index = i;
				dynamic_tokens[dynamic_tokens_count] = strdup(dest_url_tokens[t]);
				dynamic_tokens_count++;
				continue;
			}
			if (strcmp(urls[i].tokens[t],dest_url_tokens[t]) == 0){
				best_url.index = i;
				continue;
			}
			else{
				best_url = (struct MatchedUrl){.index=-1,.arguments=NULL,.arguments_count=0};
				printf("DEBUG:#2 URL [%s] rejected\n",urls[i].path);
				break;
			}
		}
		// found a view that match URL
		if(best_url.index != -1){
			best_url.arguments = dynamic_tokens;
			best_url.arguments_count = dynamic_tokens_count;
			return best_url;
		}
		else{
			best_url = (struct MatchedUrl){.index=-1,.arguments=NULL,.arguments_count=0};
		}
		free(dynamic_tokens);
	}
	return best_url;
}

Url *create_url(char* path,HttpResponse *(*view)(HttpRequest request)){
	Url *url = malloc(sizeof(Url));
	int token_counts;
	url->path = path;
	url->view = view;
	url->tokens = tokenizeString(path,"/",&token_counts);
	url->tokens_count = token_counts;

	// printf("\nTokens: ");
	// for(int i =0;i<(token_counts);i++){
	// 	printf("[%s] ",url->tokens[i]);
	// }
	// printf("\n");
	
	return url;
}

HttpRequest parse_request(char *request){
	int request_len = strlen(request);
	char *request_metadata = NULL;
	char *request_headers = NULL;
	int request_headers_count = 0;
	char *request_body = NULL;
	char *crlf = "\r\n";
	int crlf_len = strlen(crlf);
	int last_section =0;
	HttpRequest http_request = {
									.version=NULL,
									.method=NULL,
									.url=NULL,
									.headers=NULL,
									.body=NULL
								};

	for(int i=0;i<=request_len;i++){
		// check it request length is enough to have 2 CRLF at itself as end the headers
		if(
			request_metadata &&
			i + crlf_len*2 <= request_len &&
			strcmp(str_slice(request,i,i+crlf_len), crlf) == 0 &&
			strcmp(str_slice(request,i+crlf_len,i+(crlf_len*2)), crlf) == 0
		){
			request_headers= str_slice(request,last_section,i);
			last_section =  i;
			continue;
		};
		if(!request_metadata && i + crlf_len <=request_len && strcmp(str_slice(request,i,i+crlf_len),crlf) ==0){
			request_metadata = str_slice(request,last_section,i);
			last_section =  i;
			continue;
		};
		if(request_metadata && http_request.headers){
			request_body = str_slice(request,last_section,request_len);
			break;
		}
	};

	char *metadata = request_metadata;
	http_request.method = strsep(&metadata," ") ? : "Unknown";
	http_request.url = strsep(&metadata," ") ? : "Unknown";
	http_request.version = strsep(&metadata," ") ? : "Unknown";
	

	char **headers = tokenizeString(request_headers,crlf,&request_headers_count);
	http_request.headers = malloc(sizeof(Header)*request_headers_count );
	for(int i=0;i<request_headers_count;i++){
		int count=0;
		char **header_items =  tokenizeString(headers[i],": ",&count);
		Header header_obj = (Header){
		header_obj.key = header_items[0],
		header_obj.value =header_items[1],
		};
		http_request.headers[i] = header_obj;
	}
	http_request.headers_count = request_headers_count;

	return http_request;


}

HttpResponse *dispatch_request(HttpRequest request,Url urls[],int urls_count){
	struct MatchedUrl matched = match_url(request.url,urls,urls_count);
	if(matched.index == -1){
		HttpResponse *response = malloc(sizeof(HttpResponse));
		response->status="404 Not Found";
		response->headers="Content-Type: text/plain\r\nContent-Length: 0\r\n";
		response->body="";
		return response;
	}
	request.arguments=matched.arguments;
	request.arguments_count=matched.arguments_count;
	return urls[matched.index].view(request);
}

// VIEWS
HttpResponse *index_view(HttpRequest request){
	char header[1024];
	char *body = "";
	int body_size = strlen(body);
	snprintf(header,sizeof(header),"",body_size);
	HttpResponse *response = malloc(sizeof(HttpResponse));
	response->status="200 OK";
	response->headers=strdup(header);
	response->body=body;
	return response;
};


HttpResponse *echo_view(HttpRequest request){
	char header[1024];
	char *body = request.arguments[0];
	int body_size = strlen(body);
	snprintf(header,sizeof(header),"Content-Type: text/plain\r\nContent-Length: %d\r\n",body_size);
	HttpResponse *response = malloc(sizeof(HttpResponse));
	response->status="200 OK";
	response->headers=strdup(header);
	response->body=body;
	return response;
};
HttpResponse *user_agent_view(HttpRequest request){
	char header[1024];
	char *body ;
	
	for(int i=0;i<request.headers_count;i++){
		if( strcmp("User-Agent",request.headers[i].key) == 0){
			body = request.headers[i].value;
			break;
		}
	}

	int body_size = strlen(body);
	snprintf(header,sizeof(header),"Content-Type: text/plain\r\nContent-Length: %d\r\n",body_size);
	HttpResponse *response = malloc(sizeof(HttpResponse));
	response->status="200 OK";
	response->headers=strdup(header);
	response->body=body;
	return response;
};

HttpResponse *files_view(HttpRequest request){
	char header[1024];
	char *fileName = request.arguments[0];
	FILE *file =fopen(fileName,"r");
	long body_size = 0;
	char *body;



	if(file != NULL){

		// findout the file length
		fseek(file,0,SEEK_END);
		body_size = ftell(file);
		fseek(file,0,SEEK_SET);

		// store file content intro string
		body = malloc(body_size + 1);
		fgets(body,body_size,file);
		body[body_size] = '\0';
		fclose(file);

	}
	else{
		body = strdup("");
	};
	

	snprintf(header,sizeof(header),"Content-Type: application/octet-stream\r\nContent-Length: %d\r\n",body_size);
	HttpResponse *response = malloc(sizeof(HttpResponse));
	response->status=(file != NULL) ? "200 OK" : "404 Not Found"; 
	response->headers=strdup(header);
	response->body=body;
	return response;
};

void *process_request(void *arg){
	struct ProcessBlock *pBlock = (struct ProcessBlock*)arg;
	int client = pBlock->client;
	Url *urls = pBlock->urls;
	int urls_count = pBlock->urls_count;

	int request_size = 10240;
	char *request = malloc(request_size);
	int received_len = recv(client,request,request_size ,0);
	request[received_len]= '\0';

	HttpRequest http_request = parse_request(request);
	HttpResponse *http_response =  dispatch_request(http_request,urls,urls_count);
	char *response;
	response = make_response(http_response->status,http_response->headers,http_response->body);
	if (!response){
		return NULL;
	}
	send(client,response,strlen(response),0);
	free(response);
	close(client);
	return NULL;
}



int main() {
	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);
	int server_port = 4221;

	Url urls[] ={
		*create_url("/",index_view),
		*create_url("/echo/?",echo_view),
		*create_url("/user-agent",user_agent_view),
		*create_url("/files/?",files_view),
	};
	int urls_count = 4;
	

	int server_fd, client_addr_len;
	struct sockaddr_in client_addr;
	
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}
	
	// Since the tester restarts your program quite often, setting SO_REUSEADDR
	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		return 1;
	}
	
	struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
									 .sin_port = htons(server_port),
									 .sin_addr = { htonl(INADDR_ANY) },
									};
	
	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		
		return 1;
	}
	
	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0) {
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}
	
	printf("Waiting for a client to connect...\n");
	client_addr_len = sizeof(client_addr);
	while(1){
		int client = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
		printf("Client connected\n");
		pthread_t thread_1;
		struct ProcessBlock pBlock = (struct ProcessBlock){client,urls,urls_count};
		pthread_create(&thread_1,NULL,process_request,&pBlock);
	
		printf("Send Response\n");
		// pthread_join(thread_1,NULL);
	}

	close(server_fd);

	return 0;
}

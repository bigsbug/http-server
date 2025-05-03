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

#include "http.h"
#include "views.h"
#include "utils.h"
#include "middlewares.h"

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

char* make_header_response(char* status,Header* headers_obj,int headers_count){
	// Avoid Null inputs
	if( !status){
		return NULL;
	}
	char *http_version = "HTTP/1.1";
	char *crlf = "\r\n";
	char* headers = header_as_string(headers_obj,headers_count);
	// we had a padding after our version
	int http_version_len = strlen(http_version) + 1;
	// we had two crlf on our response
	int crlf_len = strlen(crlf)*2;
	int status_len = strlen(status);
	int header_len = strlen(headers);
	int nullTerminator_len = 2;
	int total_len = http_version_len + crlf_len + status_len + header_len  + nullTerminator_len;
	
	char *response = malloc(total_len);
	// malloc memory failed
	if (!response){
		return NULL;
	};

	int total_wrote = snprintf(response,total_len,"%s %s%s%s%s",http_version,status,crlf,headers,crlf);
	// check response is overflow
	if (total_wrote == 0 || total_wrote >= total_len){
		return NULL;
	};

	return response;
};

struct MatchedUrl match_url(char* dest_url,char*request_method,Url urls[],int urls_count){

	char **dest_url_tokens;
	int url_tokens_count;
	struct MatchedUrl best_url = {.index=-1,.arguments=NULL,.arguments_count=0} ;
	printf("METHOD: %s\n",request_method);
	for(int i=0;i<urls_count;i++){
		if(strcmp(request_method, urls[i].method) != 0){
			// printf("URL [%s] rejected by Rqe Method [%s] and Url Method [%s]\n",urls[i].path,request_method,urls[i].method);
			continue;
		}

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
				// printf("DEBUG:#2 URL [%s] rejected\n",urls[i].path);
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

Url *create_url(char* path,char*method,HttpResponse *(*view)(HttpRequest request)){
	Url *url = malloc(sizeof(Url));
	int token_counts;
	url->path = path;
	url->method = method;
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
								.version="",
								.method="",
								.url="",
								.headers=NULL,
								.body=""
								};

	for(int i=0;i<=request_len;i++){
		if(request_metadata && request_headers){
			// +4 = \r\n\r\n
			request_body = str_slice(request,last_section+4,request_len);
			break;
		}

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
		
	};

	char *metadata = request_metadata ;
	http_request.method = (metadata != NULL) ? strsep(&metadata, " ") : "Unknown";
	http_request.url    = (metadata != NULL) ? strsep(&metadata, " ") : "Unknown";
	http_request.version= (metadata != NULL) ? strsep(&metadata, " ") : "Unknown";


	char **headers = tokenizeString(request_headers ?: "",crlf,&request_headers_count ?: 0);
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
	http_request.body =request_body;

	return http_request;


}

HttpResponse *dispatch_request(HttpRequest request,Url urls[],int urls_count){
	struct MatchedUrl matched = match_url(request.url,request.method,urls,urls_count);
	if(matched.index == -1){
		printf("URL NOT FOUND\n");
		HttpResponse *response = malloc(sizeof(HttpResponse));
		response->status="404 Not Found";
		int headers_count = 1;
		response->headers = malloc(sizeof(Header)* headers_count);
		response->headers[0] = (Header){.key="Content-Type","text/plain"};
		response->body="";
		return response;
	}
	request.arguments=matched.arguments;
	request.arguments_count=matched.arguments_count;
	return urls[matched.index].view(request);
}

void *process_request(void *arg){
	struct ProcessBlock *pBlock = (struct ProcessBlock*)arg;
	int client = pBlock->client;
	Url *urls = pBlock->urls;
	int urls_count = pBlock->urls_count;
	while(1){
		int request_size = 10240;
		char *request = malloc(request_size);
		int received_len = recv(client,request,request_size ,0);
		request[received_len]= '\0';
		HttpRequest http_request = parse_request(request);

		if(received_len ==0){
			printf("Connection Closed %d\n",client);
			close(client);
			return NULL;
		}

		
		printf("URL: %s %s\n",http_request.method,http_request.url);
		HttpResponse *http_response =  dispatch_request(http_request,urls,urls_count);
	
		// middleware_add_encoding(http_response,&http_request);
		// middleware_add_content_length(http_response,NULL);
		middleware_add_connection_status(http_response,&http_request);
		
		char *response_header = make_header_response(http_response->status,
								http_response->headers,
								http_response->headers_count);
		if (!response_header){
			return NULL;
		}
		send(client,response_header,strlen(response_header),0);
		send(client,http_response->body,http_response->body_length,0);
		free(response_header);

		for(int i=0;i<http_response->headers;i++){
			if(
				strcmp(http_response->headers[i].key,"Connection") == 0 &&
				strcmp(http_response->headers[i].value,"close") == 0 
			 ){
				close(client);
				printf("Connection Closed %d\n",client);
				return NULL;
			 }


		};

	}

	return NULL;
}


int main() {
	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);
	int server_port = 4221;

	Url urls[] ={
		*create_url("/","GET",index_view),
		*create_url("/echo/?","GET",echo_view),
		*create_url("/user-agent","GET",user_agent_view),
		*create_url("/files/?","GET",files_view),
		*create_url("/files/?","POST",post_files_view),
	};
	int urls_count = sizeof(urls) / sizeof(Url);
	

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

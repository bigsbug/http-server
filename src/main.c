#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>

struct HttpResponse{
	char* status;
	char* headers;
	char* body;
};
struct HttpRequest{
	char *version;
	char *method;
	char *url;
	char *headers;
	char *body;
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
	if( !status || !headers || !body){
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

struct HttpRequest parse_request(char *request){
	int request_len = strlen(request);
	char *request_metadata = NULL;
	char *request_headers = NULL;
	char *request_body = NULL;
	char *crlf = "\r\n";
	int crlf_len = strlen(crlf);
	int last_section =0;
	struct HttpRequest http_request = {
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
			http_request.headers = str_slice(request,last_section,i);
			last_section =  i;
			continue;
		};
		if(!request_metadata && i + crlf_len <=request_len && strcmp(str_slice(request,i,i+crlf_len),crlf) ==0){
			request_metadata = str_slice(request,last_section,i);
			last_section =  i;
			continue;
		};
		if(request_metadata,http_request.headers){
			request_body = str_slice(request,last_section,request_len);
			break;
		}
	};
	char *metadata[3];
	int metadata_len = 0;
	int request_metadata_len = strlen(request_metadata);
	char *data = NULL;
	last_section=0;

	// spilt by space
	for(int i=0;i<=request_metadata_len;i++){
		data = str_slice(request_metadata,i,i+1);
		if(strcmp(data," ")==0 || i>=request_metadata_len){
			data = str_slice(request_metadata,last_section,i);
			last_section = i+1; // +1 is space on string
			metadata[metadata_len] = data;
			metadata_len++;
		};
	}

	if(metadata_len >= 0)
		http_request.method = metadata[0];
	else
		http_request.method = "Unknown";

	if(metadata_len >= 1)
		http_request.url = metadata[1];
	else
		http_request.url = "/";

	if(metadata_len >= 2)
		http_request.version = metadata[2];
	else
		http_request.version = "Unknown";

	return http_request;


}

int main() {
	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);

	// You can use print statements as follows for debugging, they'll be visible when running tests.
	printf("Logs from your program will appear here!\n");

	// Uncomment this block to pass the first stage

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
									 .sin_port = htons(4221),
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
	
	int client = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
	printf("Client connected\n");

	int request_size = 10240;
	char *request = malloc(request_size);
	int received_len = recv(client,request,request_size ,0);
	request[received_len]= '\0';

	struct HttpRequest http_request = parse_request(request);

	char *response;
	if (http_request.method == "GET" && http_request.url == "/")
		response = make_response("200 OK","","");
		if (!response){
			return 2;
		}
	else
		response = make_response("404 Not Found","","");
		if (!response){
			return 2;
		}


	send(client,response,strlen(response),0);

	printf("Send Response\n");

	close(server_fd);

	return 0;
}

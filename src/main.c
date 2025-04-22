#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
struct HttpResponse{
	char* status;
	char* headers;
	char* body;
};

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

	char *response = make_response("200 OK","","");
	if (!response){
		return 2;
	}
	
	send(client,response,strlen(response),0);

	printf("Send Response\n");

	close(server_fd);

	return 0;
}

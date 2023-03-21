#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <wait.h>
#include <stdlib.h>
#include <time.h>

#define MAX_BUF 8192
#define LOG_ACCESS 0
#define LOG_ERROR 1

char root_path[64];
char index_path[64];

void write_log(int type, char* msg1, char *msg2) {
	
	char time_string[80];
	time_t curr_time = time(NULL);
	memset(time_string, 0, sizeof(time_string));
	strcat(time_string, ctime(&curr_time));
	// time_string = ctime(&curr_time);
	time_string[strlen(time_string)-1] = 0;
	
	FILE *log_file;
	if (type == LOG_ACCESS) {
		log_file = fopen("../logs/access.log", "a");
	} else if (type == LOG_ERROR) {
		log_file = fopen("../error.log", "a");
	} else {
		perror("Error opening specified log file");
		return;
	}

	fprintf(log_file, "[%s] %s %s\n", time_string, msg1, msg2);
	fclose(log_file);
}

int import_httpd_conf(int *simul_cons, int *port) {

	printf("importing server configuration file...\n");
	write_log(LOG_ACCESS, "Importing server configuration file", "../conf/httpd.conf");
	FILE *fd = fopen("../conf/httpd.conf", "r");

	if (fd == NULL) {
		write_log(LOG_ERROR, "Could not open server configuration file", "");
		exit(EXIT_FAILURE);
	}

	if (fscanf(fd, "%d", simul_cons) < 0 ||
		fscanf(fd, "%s", root_path) < 0 ||
		fscanf(fd, "%s", index_path) < 0 ||
		fscanf(fd, "%d", port) < 0) {
		write_log(LOG_ERROR, "Could not read server configuration file correctly.", "Perhaps format issue?");
		perror("Error importing server configuration file because ");
		exit(EXIT_FAILURE);
	}
	
	fclose(fd);
	return 0;
}

char* get_content_type(char *file_path) {

	char *file_name = strtok(file_path, ".");	
	char *file_ext = strtok(NULL, " ");
	
	if (strcmp(file_ext, "jpg") == 0 ||
		strcmp(file_ext, "jpeg") == 0 ) {
		return "image/jpeg";
	} else if (strcmp(file_ext, "gif") == 0) {
		return "image/gif";
	} else if (strcmp(file_ext, "html") == 0 ||
			   strcmp(file_ext, "htm") == 0) {
		return "text/html";
	} else {
		return "text/plain";
	}
}

int send_response_header(int client_socket, int code, char *content_type, int content_length) {
	
	char resbuf[MAX_BUF];
	if (code == 404) {
		strcat(resbuf, "HTTP/1.0 404 Not Found\r\n");
	} else if (code == 200) {
		strcat(resbuf, "HTTP/1.0 200 OK\r\n");
	} else if (code == 501) {
		strcat(resbuf, "HTTP/1.0 501 Not Implemented\r\n");
	}else {
		strcat(resbuf, "HTTP/1.0 500 Internal Server Error\r\n");
	}

	char tmp[MAX_BUF];
	sprintf(tmp, 
				"Content-Type: %s\r\n"
				"Content-Length: %d\r\n"
				"Connection: close\r\n"
				"Server: LiteSpeed\r\n"
				"Date: Sat, 28 Nov 2009 04:36:25 GMT\r\n"	
				"Last-Modified: Mon, 10 April 2004 09:26:15 GMT\r\n"
				"Expires: Mon, 22 Jan 2010 01:00:00 GMT\r\n"
				"\r\n",
				content_type, content_length
	); // hard coded times for testing

	strcat(resbuf, tmp);

	printf("sent response header: %s\n",resbuf);
	send(client_socket, resbuf, strlen(resbuf), 0);
	return 0;
}


int pass_to_cgi(int client_socket, char *http_method, char *args) {
	// close stdout file descriptor
	close(1); 
	// redirect default stdout file descriptor to be client's socket descriptor
	dup2(client_socket, 1);

	setenv("QUERY_STRING", args, 1);

	// check user's python installation
	char *python_bin;
	if (access("/usr/bin/python3", X_OK) == 0) {
		python_bin = "python3";
	} else if (access("/usr/bin/python", X_OK) == 0) {
		python_bin = "python";
	} else {
		write_log(LOG_ERROR, "Could not find a valid python or python3 binary.", "Exiting...");
		perror("Could not find a valid python or python3 binary ");
		exit(EXIT_FAILURE);
	}

	write_log(LOG_ACCESS, "Found valid python binary:", python_bin);

	int res;
	if (strcmp(http_method, "GET") == 0) {
		// prep args for exec call
		char *cgi_args[] = {python_bin, "../cgi-bin/GET.cgi", args, NULL};
		write_log(LOG_ACCESS, "Launching GET CGI script", "");
		res = execvp(python_bin, cgi_args);
	} else {
		char *cgi_args[] = {python_bin, "../cgi-bin/POST.cgi", args, NULL};
		write_log(LOG_ACCESS, "Launching POST CGI script", "");
		res = execvp(python_bin, cgi_args);
	}

	// will never be reached if exec works
	if (res < 0) {
		char *res_body = "Error: HTTP 500 - Internal Server Error\n";
		write_log(LOG_ERROR, "Could not execute CGI binary.", res_body);
		send_response_header(client_socket, 500, "text/plain", strlen(res_body));
		send(client_socket, res_body, strlen(res_body), 0);
	}

	return 0;
}

int process_client_socket(int client_socket) {
	
	ssize_t ssize;
	char *http_method;
	char *file_path;
	char buffer[MAX_BUF];
	char buffer_cpy[MAX_BUF];	// copy of buffer for post body
	char *response;

	memset(buffer, 0, sizeof(buffer));

	// write http request from socket to buffer
	ssize = recv(client_socket, buffer, sizeof(buffer), 0);
	memcpy(buffer_cpy, buffer, sizeof(buffer));

	printf("Recieved \n%s\nfrom socket\n", buffer);

	// parse http request header elements based on expected positioning
	http_method = strtok(buffer, " ");
	file_path = strtok(NULL, " ");
	
	if (strcmp(file_path, "/index.html") == 0 ||
		strcmp(file_path, "/") == 0) {
		strcpy(root_path, index_path);
	} else {
		// append file path to conf root path if not cgi file request
		if (strstr(file_path, ".cgi") == NULL) {
			strcat(root_path, file_path);
		} else { // pass to CGI
			printf("method: %ss\n", http_method);
			char *cgi_args;
			if (strcmp(http_method, "GET") == 0) {
				write_log(LOG_ACCESS, "Received GET request for ", file_path);
				
				cgi_args = strtok(file_path, "?");
				cgi_args = strtok(NULL, " ");
				
				pass_to_cgi(client_socket, http_method, cgi_args);
			} else if (strcmp(http_method, "POST") == 0) {
				cgi_args = strstr(buffer_cpy, "\r\n\r\n");
				cgi_args += 4;
				write_log(LOG_ACCESS, "Recieved POST request with parameters ", cgi_args);
				// printf("post args: %s\n", cgi_args);
				pass_to_cgi(client_socket, http_method, cgi_args);
			} else {
				// write HTTP 404 response header
				char *res_body = "Error: HTTP 501 - Not an implemented request\n";
				write_log(LOG_ERROR, res_body, "");
				send_response_header(client_socket, 501, "text/plain", strlen(res_body));
				send(client_socket, res_body, strlen(res_body), 0);
			}		
		
		}
	}

	// printf("requested file: %s\n", file_path);
	// printf("server path to file: %s\n", root_path);
	
	// attempt to open file and write contents to client_socket
	FILE *fd = fopen(root_path, "rb");
	if (fd == NULL) {
		// write HTTP 404 response header
		char *res_body = "Error: HTTP 404 - File not found\n";
		write_log(LOG_ERROR, res_body, "");
		send_response_header(client_socket, 404, "text/plain", strlen(res_body));
		// write body of 
		send(client_socket, res_body, strlen(res_body), 0);
	}

	char *content_type = get_content_type(file_path);
	
	// seek to EOF to get position, corresponding to file's size
	fseek(fd, 0, SEEK_END);
	ssize_t file_size = ftell(fd);
	rewind(fd);

	send_response_header(client_socket, 200, content_type, file_size);

	// read file into buffer
	char file_buffer[file_size];
	file_size = fread(file_buffer, 1, sizeof(file_buffer), fd);	

	// write data in buffer to socket
	ssize_t bytes_sent = write(client_socket, file_buffer, file_size);
	write_log(LOG_ACCESS, "Sending requested file to client:", root_path);

	if (file_size != bytes_sent) {
		write_log(LOG_ERROR, "Error sending complete file", "");
		perror("Error sending complete file.");
	}

	fclose(fd);
}

// clean up zombie children
void sigchld_reaper(int sig) {
	int status;
	while (wait3(&status, WNOHANG, (struct rusage *)0) >= 0) {}
}

int main() {

	write_log(LOG_ACCESS, "Starting webserver...", "");

	int res;
	int simul_cons, port;
	import_httpd_conf(&simul_cons, &port);

	printf("max simul. cons.: %d\n", simul_cons);
	printf("root path:        %s\n", root_path);
	printf("index html path:  %s\n", index_path);
	printf("port:             %d\n", port);

	int server_socket, client_socket;
	struct sockaddr_in sin;
	int sin_size = sizeof(struct sockaddr_in);
	
	// zero out all byte values of sin
	// bzero((char *) &sin, sizeof(sin));
	memset(&sin, 0, sizeof(sin));

	sin.sin_port 		= htons(port);
	sin.sin_family 		= AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;

	// make a socket descriptor for server
	server_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (server_socket < 0) {
		write_log(LOG_ERROR, "Could not create socket.", "Exiting...");
		perror("Could not create socket because ");
		exit(EXIT_FAILURE);
	}
	
	// get rid of need for bind to timeout
	int optval = 1;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	res = bind(server_socket, (struct sockaddr *) &sin, sizeof(sin));
	if (res < 0) {
		write_log(LOG_ERROR, "Could not bind to socket.", "Exiting...");
		perror("Could not bind socket because ");
		exit(EXIT_FAILURE);	
	}
	
	res = listen(server_socket, simul_cons);
	if (res < 0) {
		write_log(LOG_ERROR, "Could not listen to socket.", "Exiting...");
		perror("Could not listen to socket because ");
		exit(EXIT_FAILURE);
	}

	(void) signal(SIGCHLD, sigchld_reaper);

	// main accept connection loop
	while (1) {
		client_socket = accept(server_socket, (struct sockaddr *) &sin, &sin_size);

		if (client_socket < 0) {
			write_log(LOG_ERROR, "Could not accept client socket.", "Exiting...");
			perror("Could not accept client socket because ");
			exit(EXIT_FAILURE);
		} else {
			write_log(LOG_ACCESS, "Server accepted a connection!", "");
			printf("Server accepted a connection!\n");

			int pid = fork();			
			if (pid == 0) { 			// child process
				close(server_socket); 	// close server_socket listener

				process_client_socket(client_socket);
				exit(EXIT_SUCCESS);
			} else if (pid > 0) {		// parent process
				close(client_socket); 	// close client_socket to accept next client
			} else {
				write_log(LOG_ERROR, "Error forking server process.", "Exiting...");
				perror("Could not fork parent process because ");
				exit(EXIT_FAILURE);
			}

		}
	}

	return EXIT_SUCCESS;
}
	
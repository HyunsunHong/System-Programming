#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

char ip[16]; // 255.255.255.255
int port_num;
char opt;

void
command_line_interface(int argc, char * argv[]) {
	
	int idx = 0;
	char * temp_num = 0x0;
	char * temp;
	/*
	if(argc < 3) {
		printf("Invalid commaind line interface\n");
		exit(1);		
	}*/

	while(argv[1][idx] != ':' && idx != strlen(argv[1])) { 
		idx++;
	}

	/*
	if(idx == strlen(argv[1]) || idx + 1 == strlen(argv[1])) {  // ex) 127.0.0.1 or 127.0.0.1:
		printf("Invalid commaind line interface\n");
		exit(1);		
	}
	
	if(idx > 15) {
		printf("Invalid command line interface\n");
		exit(1);
	}*/
	for(int i = 0; i < idx; i++) {
		ip[i] = argv[1][i];
	}
	ip[idx] = '\0';

	idx++;
	temp_num = (char*)malloc((strlen(argv[1]) - idx + 1)*sizeof(char));
	temp = argv[1] + idx;
	strcpy(temp_num, temp);
	port_num = atoi(temp_num);
/*
	if(port_num < 0 || port_num > 65535) {
		printf("Invalid command line interface\n");
		exit(1);
	}

	if(strcmp(argv[2], "list") == 0) {
		if(argc != 3) {
			printf("Invalid command line interface\n");
			exit(1);
		}
	}
	else if(strcmp(argv[2], "get") == 0) {
		if(argc == 3) {
			printf("Invalid command line interface\n");
			exit(1);
		}
	}
	else if(strcmp(argv[2], "put") == 0) {
		if(argc == 3) {
			printf("Invalid command line interface\n");
			exit(1);
		}
	}
	else {
		printf("Invalid command line interface\n");
		exit(1);
	}
       		*/ 
	free(temp_num);
}

void 
client(int * sock_fd, struct sockaddr_in * serv_addr, ssize_t addrlen) {
	(*(sock_fd)) = socket(AF_INET, SOCK_STREAM, 0);
	if(*(sock_fd) <= 0) {
		perror("socket failed: ");
		exit(EXIT_FAILURE);
	}

	memset(serv_addr, '0', addrlen);
	serv_addr->sin_family = AF_INET;
	serv_addr->sin_port = htons(port_num);
	if(inet_pton(AF_INET, ip, &(serv_addr->sin_addr)) <= 0) {
		perror("inet_pton failed: ");
		exit(EXIT_FAILURE);
	}

	if(connect((*sock_fd), (struct sockaddr *) serv_addr, addrlen) < 0) {
		perror("connected failed: ");
		exit(EXIT_FAILURE);
	}
	
}


void
send_data(int sock_fd, void * ptr, ssize_t len) {

	ssize_t s = 0;
	void * data = ptr;	
	
	while(len > 0 && (s = send(sock_fd, data, len, 0)) > 0) {
		data += s;
		len -= s;
	}

}

void
send_put(int sock_fd, char * filename) {
	 
	struct stat buf;
	
	int file_stat;
	unsigned char fname_len;
	unsigned long content_len;
	
	if(stat(filename, &buf) == -1 || S_ISDIR(buf.st_mode)) {
		file_stat = -1;
		send_data(sock_fd, (void*)(&file_stat), sizeof(file_stat));
	}
	else {
		file_stat = buf.st_mode;
		fname_len = strlen(filename);
		content_len = buf.st_size;
		
		send_data(sock_fd, (void*)(&file_stat), sizeof(file_stat));
		send_data(sock_fd, (void*)(&fname_len), sizeof(fname_len));
		send_data(sock_fd, (void*)(&content_len), sizeof(content_len));
		
		send_data(sock_fd, (void*)filename, strlen(filename));

		int fd = open(filename, O_RDONLY);
		int read_bites;
		char buf[1024];
		while((read_bites = read(fd, buf, 1024)) > 0) {
			send_data(sock_fd, (void*)buf, read_bites);
		}
		close(fd);
	}

}


void
receive_list(int sock_fd) {
	int s;
	char fl;

	while(1) {
		s = recv(sock_fd, &fl, 1, 0);
	       	if(s <= 0) {
			break;
		}

		int len = fl + 1;
		char * data = malloc(sizeof(char)*(len));
		char * orig = data;
		
		while(len > 0 && (s = recv(sock_fd, data, len, 0)) > 0) {
			data += s;
			len -= s;
		}

		printf("%s\n", orig);
		free(orig);
	}
}


void
receive_get(int sock_fd) {
	ssize_t s;
	unsigned char f_state;
	unsigned char fname_len;
	unsigned long * content_len;

	while(1) {
		s = recv(sock_fd, &f_state, 1, 0);
		if(s <= 0) {
			break;
		}
		
		if(f_state == 0) {
			continue;
		}

		s = recv(sock_fd, &fname_len, 1, 0);
		
		void * temp = malloc(8);
		void * data = temp;
		int len = 8;
		while(len > 0 && (s = recv(sock_fd, data, len, 0))) {
			data += s;
			len -= s;	
		}	
		content_len = (unsigned long*)temp;
		
		//printf("file receive -> f_state: %d, fname_len: %d, content_len: %lu\n", f_state, fname_len, (*content_len)); 
		
		char * fname = (char*)malloc(2 + fname_len + 1);
		data = fname + 2;
		fname[fname_len + 2] = 0x0; 
		strcpy(fname, "./");
		while(fname_len > 0 && (s = recv(sock_fd, data, fname_len, 0)) > 0){
			data += s;
			fname_len -= s;
		}
		
		void * temp1 = malloc(4);
		int * mode;
	       	data = temp1;
		len = 4;
		while(len > 0 && (s = recv(sock_fd, data, len, 0)) > 0) {
			data += s;
			len -= s;
		}
		mode = (int*)temp1;
		
		int fd = open(fname, O_RDWR|O_CREAT, (*mode));
		char buf[1024];
		while((*content_len) > 0 && (s = recv(sock_fd, buf, (*content_len), 0)) > 0) {
			write(fd, buf, s);
			(*content_len) -= s;
		}	
		
		close(fd);
		free(temp1);
		free(temp);
		free(fname);
	}
}
	


int main(int argc, char * argv[]) {
	
	if(argc < 3){
		printf("Invalid command line interface\n");
		exit(1);
	}

	int sock_fd;
	struct sockaddr_in serv_addr;
	ssize_t addrlen = sizeof(serv_addr);
	
	command_line_interface(argc, argv);
	
	client(&sock_fd, &serv_addr, addrlen); 
	
	char * request = malloc(strlen(argv[2]) + 1);
	strcpy(request, argv[2]);
	char * token = strtok(argv[2], " ");	
	if(strcmp(token, "list") == 0) {
		send_data(sock_fd, (void *)(request), strlen(request)) ;
		shutdown(sock_fd, SHUT_WR);
		receive_list(sock_fd);
	}
	if(strcmp(token, "get") == 0) {
		send_data(sock_fd, (void *)(request), strlen(request)) ;
		shutdown(sock_fd, SHUT_WR);
		receive_get(sock_fd);
	}
	if(strcmp(token, "put") == 0) {
		//send_data(sock_fd, (void *)(token), strlen(token) + 1);
		//sleep(1);
		char * opt = "put ";
		send_data(sock_fd, (void *)(opt), strlen(opt));

		//request = malloc(strlen("put ") + 1);
		//strcpy(request, "put ");

		token = strtok(0x0, " ");
		while(token != 0x0) {
			send_put(sock_fd, token);
			token = strtok(0x0, " ");
		}		
		

		//send_data(sock_fd, (void *)(request), strlen(request) + 1);
		//shutdown(sock_fd, SHUT_WR);
	}
	
	
	free(request);
	return 0;
		
}

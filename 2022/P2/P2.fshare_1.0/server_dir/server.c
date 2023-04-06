#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

int port_num;
char * shared_directory = 0x0;

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
send_list(int conn) {
	DIR * dir = opendir(shared_directory);
	struct dirent * dir_entry;

	while((dir_entry = readdir(dir)) != 0x0) {
		char * name = dir_entry->d_name;
		if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
			continue;
		}
		struct stat st;
		stat(name, &st);
		if(S_ISDIR(st.st_mode)) {
			continue;
		}

		typedef struct {
			unsigned char fname_len;
			char fname[256];
		}a_file;

		a_file af;
		af.fname_len = (unsigned char)strlen(name);
		strcpy(af.fname, name);

		int s;
		a_file *a = &af;
		while(a < &af + af.fname_len + 2) {
			s = send(conn, a, af.fname_len + 2 - (a - &af), 0);
			if(s < 0) {
				perror("Send failed");
				exit(1);
			}
			a += s;
		}
				
	}
	closedir(dir);
	shutdown(conn, SHUT_WR);
}

void
send_get(int conn, char * filename) {
	char * path = (char*)malloc((strlen(shared_directory) + strlen(filename) + 1) * sizeof(char));
	char * temp = path;
	strcpy(path, shared_directory);
	temp += strlen(shared_directory);
	strcpy(temp, filename);
	
	unsigned char f_state;
	unsigned char fname_len;
	unsigned long content_len;

	struct stat buf;
	int s;
	int len;
	
	if(stat(path, &buf) == -1 || S_ISDIR(buf.st_mode)) {
		f_state = 0;
		send_data(conn,(void*)&f_state, 1);
	}
	else {
		f_state = 1;
		fname_len = strlen(filename);
		content_len = buf.st_size;

		send_data(conn, (void*)&f_state, sizeof(f_state));
		send_data(conn, (void*)&fname_len, sizeof(fname_len));
		send_data(conn, (void*)&content_len, sizeof(content_len)); 

		
		send_data(conn, (void*)filename, strlen(filename));
		send_data(conn, (void*)(&buf.st_mode), sizeof(buf.st_mode));
		int fd = open(path, O_RDONLY);
		int read_bites;
		char buff[1024];
		while((read_bites = read(fd, buff, 1024)) > 0) {
			send_data(conn,(void*)buff, read_bites);
		}
		close(fd);
	}

	free(path);
}

void
receive_put(int sock_fd) {
	
	ssize_t s;
	int file_stat;
	unsigned char fname_len;
	unsigned long content_len;
	int len = 0;
		
	while(1) {
	
	void * file_stat_buf = malloc(sizeof(int));
	char * data = file_stat_buf;
	len = 4;
	while(len > 0 && (s = recv(sock_fd, data, len, 0)) > 0) {
		data += s;
		len -= s;		
	}
	if(s <= 0) {
		free(file_stat_buf);
		break;
	}
	file_stat = (*((int *)file_stat_buf));
	free(file_stat_buf);
	
	if(file_stat == -1) {
		continue;
	}

	recv(sock_fd, &fname_len, 1, 0);
	
	void * temp = malloc(8);
	data = temp;
	int len = 8;
	while(len > 0 && (s = recv(sock_fd, data, len, 0)) > 0) {
		data += s;
		len -= s;
	}
	content_len = (*((unsigned long*)temp));

	char * fname = (char *)malloc(fname_len + 1);
	data = fname;
	fname[fname_len] = 0x0;
	while(fname_len > 0 && (s = recv(sock_fd, data, fname_len, 0)) > 0) {
		data += s;
		fname_len -= s;
	}

	int fd = open(fname, O_RDWR|O_CREAT, file_stat);
	char buf;
	while(content_len > 0 && (s = recv(sock_fd, &buf, 1, 0)) > 0) {
		write(fd, &buf, s);
		content_len -= s;
	}
	
	free(fname);
	free(temp);
	close(fd);
	}
		
}

void* 
server_thread(void * param) {
	int s;
	int conn = (*(int *)param);
	int fd;

	char buf[1024];
	int idx = 0;

	while( (s = recv(conn, &buf[idx], 1, 0)) > 0 && buf[idx++] != ' ') {
		buf[idx] = 0x0;
	}	
	
	strtok(buf, " ");

	if(strcmp(buf, "list") == 0) {
		send_list(conn);
		shutdown(conn, SHUT_WR);
	}
	else if(strcmp(buf, "get") == 0) {
		
		char * request = 0x0;	
		char buff[1024];
		int len = 0;
		while((s = recv(conn, buf, 1023, 0)) > 0 ) {
			buff[s] = 0x0;
			request = realloc(request, len + s + 1);
			strcpy(request + len, buf);
			len += s;
		}
		
		char * token = strtok(request, " ");
		while(token != 0x0) {
			send_get(conn, token);
			token = strtok(0x0, " ");
		}
		
		shutdown(conn, SHUT_WR);
		free(request);
	}
	else if(strcmp(buf, "put") == 0) {
		shutdown(conn, SHUT_WR);
		receive_put(conn);
	}
	
	close(conn);
	close(fd);
	free(param);
	pthread_exit(0);
}

void
command_line_interface(int argc, char * argv[]) {
	int opt;
	int flag_p, flag_d = 0;
	
	while((opt = getopt(argc, argv, "p:d:")) != -1) {
		switch(opt) {
			case 'p':
				flag_p = 1;
				port_num = atoi(optarg);
				break;
			case 'd':
				flag_d = 1;
				shared_directory = optarg;
				break;
			case '?':
				printf("Invalid command");
				exit(1);
		}
	}

	if(port_num < 0 || port_num > 65536) {
		printf("Invalid port number %d\n", port_num);
		exit(1);
	}
	struct stat buf;
	if(stat(shared_directory, &buf) == -1) {
		perror("Invalid share directory");
		exit(1);
	}
	if(!S_ISDIR(buf.st_mode)) {
		printf("Invalid shared directory\n");
		exit(1);
	}
}

void
socket_and_bind(int * listen_fd, struct sockaddr_in * address, ssize_t addrlen) {

	*(listen_fd) = socket(AF_INET, SOCK_STREAM, 0);
	if(*(listen_fd) == 0) {
		perror("socket failed : ");
		exit(EXIT_FAILURE);
	}
		
	memset(address, '0', addrlen);
	address->sin_family = AF_INET;
	address->sin_addr.s_addr = INADDR_ANY;
	address->sin_port = htons(port_num);
	if(bind(*(listen_fd), (struct sockaddr *)address, addrlen) < 0) {
		perror("bind failed: ");
		exit(EXIT_FAILURE);
	}
}

void
server(int listen_fd, struct sockaddr_in * address, ssize_t addrlen) {
	
	while(1) {
		if(listen(listen_fd, 16) < 0) {
			perror("listen failed : ");
			exit(EXIT_FAILURE);
		}
		
		int * new_socket = malloc(sizeof(int));
		(*new_socket) = accept(listen_fd, (struct sockaddr *)address, (socklen_t*)&addrlen);
		if((*new_socket) < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}

		pthread_t tid;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		
		pthread_create(&tid, &attr, server_thread, new_socket);
	}
}

int
main(int argc, char * argv[]) {
	
	int listen_fd;
	struct sockaddr_in address;
	ssize_t addrlen = sizeof(address);

	command_line_interface(argc, argv);

	socket_and_bind(&listen_fd, &address, addrlen);
	
	server(listen_fd, &address, addrlen);
	
	return 0;
}


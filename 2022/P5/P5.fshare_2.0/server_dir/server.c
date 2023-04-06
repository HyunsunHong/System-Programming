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
#include <stddef.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <signal.h>

#define WRITE 0
#define DELETE 1
#define END 2
#define RENAME 3

int port_num;
char * mode = 0x0;
char * shared_directory = 0x0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // mutex for client's list

struct msg {
	char option;
	struct stat state;
	unsigned char filenamelen;
};

struct entry {
	int fd;
	LIST_ENTRY(entry) entries;
};

LIST_HEAD(listhead, entry);
struct listhead head;

void
sigterm_handler(int sig) {

        printf("fshare_server stop(portnum: %d)\n", port_num);

        exit(0);
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

int
recv_data(int sock_fd, void * ptr, ssize_t len) {
	
	for(ssize_t p = 0; p < len;) {
		ssize_t r;

		r = recv(sock_fd, (char *)ptr + p, len - p, 0);
		if(r == 0)
			return 0;
				
		p += r;
	}
	return 1; 
}
	
// called when server process is killed by sigterm signal.
void
clear_list() {
	struct entry * n1, * n2;

               
	n1 = LIST_FIRST(&head);
        while (n1 != NULL) {
             n2 = LIST_NEXT(n1, entries);
             free(n1);
             n1 = n2;
         }
         LIST_INIT(&head);
}

void
send_shared_files(int conn) {
	DIR * dir = opendir("./");
	struct dirent * dir_entry;

	while((dir_entry = readdir(dir)) != 0x0) {
		char * name = dir_entry->d_name;
		if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
			continue;
		}
		
		struct stat st;
		if(stat(name, &st) == -1) { 
			printf("stat failed\n");
		}
		if(S_ISDIR(st.st_mode)) {
			continue;
		}
		
		struct msg header;
		header.option = WRITE;
		header.state = st;
		header.filenamelen = (unsigned char)strlen(name);

		send_data(conn, (void *)(&header), sizeof(struct msg));
		send_data(conn, (void *)(name), strlen(name));
		int fd = open(name, O_RDONLY);
		ssize_t read_bites;
		char buf;
		while((read_bites = read(fd, &buf, 1)) > 0) {
			send_data(conn, (void *)&buf, read_bites);
		}
		close(fd);
	}
	
	struct msg end_header;
	end_header.option = END;
	send_data(conn, (void *)(&end_header), sizeof(struct msg)); 	

	closedir(dir);
	//shutdown(conn, SHUT_WR);
}

void
recv_and_broadcast(int conn) {
	ssize_t s;
	struct msg header;
	char * data;

	while(1) {
		// recv header
		ssize_t header_len;
		header_len = sizeof(struct msg);
		data = (char *)&header;
		while(header_len > 0 && (s = recv(conn, data, header_len, 0)) > 0) {
			header_len -= s;
			data += s;
		}
		if(header.option == END) break;
		
		// recv payload
		char * filename =(char *)malloc(header.filenamelen + 1);
		unsigned char filename_len = header.filenamelen;
		data = filename;
		filename[filename_len] = 0x0;
		while(filename_len > 0 && (s = recv(conn, data, filename_len, 0)) > 0) {
			data += s;
			filename_len -= s;
		} 	
		int fd = open(filename, O_RDWR|O_CREAT, header.state.st_mode);
		char buf;
		unsigned long long file_len = header.state.st_size;
		while(file_len > 0 && (s = recv(conn, &buf, 1, 0)) > 0) {
			write(fd, &buf, s);
			file_len -= s;
		}
		close(fd);		

		// broeadcast
		struct entry * np; 
		LIST_FOREACH(np, &head, entries)
			if(np->fd != conn) {
				printf("server: file matching broadcasting. from: %d, to : %d, file : %s, option, %d \n", conn, np->fd, filename, header.option);
				send_data(np->fd, (void *)(&header), sizeof(struct msg));
				send_data(np->fd, (void *)filename, header.filenamelen); 
				fd = open(filename, O_RDWR);
				file_len = header.state.st_size;
				
				while(file_len > 0 && (s = read(fd, &buf, 1)) > 0) {
					send_data(np->fd, (void *)&buf, s);
					file_len -= s;
				}
			}
		
		close(fd);
		free(filename);
	}
}

// match files with a connected client 
void
match_file(int conn) {
	pthread_mutex_lock(&mutex);
	
	printf("server: %d is now connected!, file matching start\n", conn);
	struct entry * n;
	n = (struct entry *)malloc(sizeof(struct entry));
	n->fd = conn;
	LIST_INSERT_HEAD(&head, n, entries);
	
	send_shared_files(conn);
	recv_and_broadcast(conn);

	printf("server: %d  file matching end\n", conn);
	pthread_mutex_unlock(&mutex);
}

void* 
server_thread(void * param) {
	
	int conn = (*(int *)param);
	match_file(conn);
	
	// debug this part (server's broadcasting thread)
	
	// broadcasting
	struct msg header;	
	while(recv_data(conn, (char *)&header, sizeof(struct msg))) {
		// recv from a client
		char * filename = 0x0;
		char * file = 0x0;
		filename = malloc(header.filenamelen + 1);
		if(header.option != DELETE) 
			file = malloc(header.state.st_size);
		
		recv_data(conn, filename, header.filenamelen);
		filename[header.filenamelen] = 0x0;
		if(header.option != DELETE) 
			recv_data(conn, file, header.state.st_size);		
				
		printf("server: got msg from %d, option %d, filename %s, filenamelen %d\n", conn, header.option, filename, header.filenamelen); 	
		pthread_mutex_lock(&mutex); 
		// modify a file as msg states
                int fd;                                 
                if(header.option == DELETE) { // no file stamp on delete. In inotify,check whethter file exist or not
                        fd = open(filename, O_RDONLY);
			if(fd != -1)
                 	       remove(filename);
                }
                else if(header.option == WRITE) {
                        fd = open(filename, O_WRONLY | O_CREAT, header.state.st_mode);

                        unsigned long long file_len = header.state.st_size;
                        ssize_t s;
			char * data = file;
                        while(file_len > 0 && (s = write(fd, data, file_len)) > 0) {
                                file_len -= s;
                                data += s;
                        }
			
		}
		// not yet rename
		
		// broadcast
		struct entry * np = 0x0;
		LIST_FOREACH(np, &head, entries)
			if(np->fd != conn) {
				printf("server: file broadcasting. from: %d, to : %d, file : %s, option, %d \n", conn, np->fd, filename, header.option);
				send_data(np->fd, (void *)(&header), sizeof(struct msg));
				send_data(np->fd, filename, header.filenamelen);				
				// Not yet rename
				if(header.option == RENAME) {
					//send_data(np->data, file, ?);	
				}
				else if(header.option == WRITE) {
					send_data(np->fd, file, header.state.st_size);	
				}	
			}		


		//free(filename);
		if(file != 0x0)
			free(file);
		pthread_mutex_unlock(&mutex);
		}
	
	
}

void
command_line_interface(int argc, char * argv[]) {
	int opt;
	int flag_p, flag_d, flag_m = 0;
	
	while((opt = getopt(argc, argv, "p:d:m:")) != -1) {
		switch(opt) {
			case 'p':
				flag_p = 1;
				port_num = atoi(optarg);
				break;
			case 'd':
				flag_d = 1;
				shared_directory = optarg;
				break;
			case 'm':
				flag_m = 1;
				mode = optarg;
				break;
			case '?':
				printf("Invalid command");
				exit(1);
		}
	}
	/*
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
	}*/

}

void
socket_and_bind(int * listen_fd, struct sockaddr_in * address, ssize_t addrlen) {


	LIST_INIT(&head);

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
	
	printf("server: on\n");	
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
	
	if(strcmp(mode, "start") == 0) {
		pid_t pid = fork();
		if(pid == 0) {
			signal(SIGTERM, sigterm_handler);
			chdir(shared_directory);	
			socket_and_bind(&listen_fd, &address, addrlen);	
			server(listen_fd, &address, addrlen);
		}
		else if(pid > 0) {
			char name[256];
			char pid_str[256];
			sprintf(name, "%d", port_num);
			sprintf(pid_str, "%d", pid);
			strcat(name, ".txt");			
			
			int fd = open(name, O_RDWR|O_CREAT, S_IRWXU);
			ssize_t len = strlen(pid_str);
			ssize_t s;
			char * data = pid_str;
			while(len > 0 && (s = write(fd, data, len))) {
				len -= s;
				data += s;
			}
			printf("fshare_server start(portnum: %d)\n", port_num);
		}
	}
	else if(strcmp(mode, "stop") == 0) {
		char name[256];
                char pid_str[256];
                sprintf(name, "%d", port_num);
                strcat(name, ".txt");

                int fd = open(name, O_RDONLY);
                char buf;
                int idx = 0;
                while(read(fd, &buf, 1) == 1) {
                        pid_str[idx++] = buf;
                }
                pid_str[idx] = 0x0;

                pid_t pid = atoi(pid_str);
                kill(pid, SIGTERM);

                remove(name);
	}
	
	return 0;
}


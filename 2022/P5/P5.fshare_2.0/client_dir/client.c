#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <stddef.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/file.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/inotify.h>

#define WRITE 0
#define DELETE 1
#define END 2
#define BUF_LEN 1000

char ip[16]; // 255.255.255.255
int port_num;
char opt;
char * mode = 0x0;
char * shared_directory = 0x0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // mutex for send queue

struct msg {
	char option;
	struct stat state;
	unsigned char filenamelen;
};

struct entry_download {
	char * name;
	LIST_ENTRY(entry_download) entries_download;
};

struct file_stamp {
	char * filename;
	struct timespec timestamp;
	LIST_ENTRY(file_stamp) entries_file_stamp;
};

struct entry_send {
	char option;
	int fd;
	char * filename;
	LIST_ENTRY(entry_send) entries_entry_send;
};

LIST_HEAD(listhead_download, entry_download);
LIST_HEAD(listhead_filestamp, file_stamp); 
LIST_HEAD(listhead_send, entry_send); 
struct listhead_download head_download;
struct listhead_filestamp head_filestamp;
struct listhead_send head_send;

void
clear_list() {
        struct entry_download * n1, * n2;

        n1 = LIST_FIRST(&head_download);
        while (n1 != NULL) {
             n2 = LIST_NEXT(n1, entries_download);
	     free(n1->name);
             free(n1);
             n1 = n2;
         }
         LIST_INIT(&head_download);	
	
        struct file_stamp * n1_1, * n2_1;
        n1_1 = LIST_FIRST(&head_filestamp);
        while (n1_1 != NULL) {
             n2_1 = LIST_NEXT(n1_1, entries_file_stamp);
	     free(n1_1->filename);
             free(n1_1);
             n1_1 = n2_1;
         }
         LIST_INIT(&head_filestamp);
	
        struct entry_send * n1_2, * n2_2;
        n1_2 = LIST_FIRST(&head_send);
        while (n1_2 != NULL) {
             n2_2 = LIST_NEXT(n1_2, entries_entry_send);
             free(n1_2);
             n1_2 = n2_2;
         }
         LIST_INIT(&head_send);
}

void
sigterm_handler(int sig) {
	
	char ip_str[17];
	for(int i = 0; i < 16; i++) {
		ip_str[i] = ip[i];
	}
	ip_str[16] = 0x0;
	printf("fshare_client stop(ip:%s, portnum: %d)\n", ip_str, port_num);	
	
	clear_list();

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

void
command_line_interface(int argc, char * argv[]) {
	int idx = 0;
	int opt;
	char * temp_num = 0x0;
	char * temp;
	char * port;
        int flag_d, flag_m, flag_p = 0;

        while((opt = getopt(argc, argv, "p:d:m:")) != -1) {
                switch(opt) {
                        case 'd':
                                flag_d = 1;
                                shared_directory = optarg;
                                break;
                        case 'm':
                                flag_m = 1;
                                mode = optarg;
                                break;
                        case 'p':
                                flag_p = 1;
                                port = optarg;
                                break;
                        case '?':
                                printf("Invalid command");
                                exit(1);
                }
        }

	while(port[idx] != ':' && idx != strlen(port)) { 
		idx++;
	}

	for(int i = 0; i < idx; i++) {
		ip[i] = port[i];
	}
	ip[idx] = '\0';

	idx++;
	temp_num = (char*)malloc((strlen(port) - idx + 1)*sizeof(char));
	temp = port + idx;
	strcpy(temp_num, temp);
	port_num = atoi(temp_num);
	free(temp_num);
}

void 
socket_and_connect(int * sock_fd, struct sockaddr_in * serv_addr, ssize_t addrlen) {
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

        int conn = *sock_fd;
        int opt_val = 1;
        if(setsockopt(conn, SOL_TCP, TCP_NODELAY, (const char *)&opt_val, sizeof(opt_val)) == -1){
                perror("setsockopt failed : ");
                exit(EXIT_FAILURE);
        }
}

void
recv_shared_files(int conn) {
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
                char * filename = (char *)malloc(header.filenamelen + 1);
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
			
		struct entry_download * n;
		n = (struct entry_download *)malloc(sizeof(struct entry_download));
		n->name = filename;
		LIST_INSERT_HEAD(&head_download, n, entries_download); 	

                close(fd);
                //free(filename);
        }
}

int
is_from_server(char * filename) {
	
	struct entry_download * np;
	LIST_FOREACH(np, &head_download, entries_download)
		if(strcmp(np->name, filename) == 0) 
			return 1;

	return 0;
}

int
is_exist_client(char * filename) {
	
	struct file_stamp * np;
	LIST_FOREACH(np, &head_filestamp, entries_file_stamp)
		if(strcmp(np->filename, filename) == 0) 
			return 1;

	return 0;
}

void
filter_and_send(int conn) {
	DIR * dir = opendir("./");
        struct dirent * dir_entry;

        while((dir_entry = readdir(dir)) != 0x0) {
                char * name = dir_entry->d_name;
                if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0 || is_from_server(name)) {
                        continue;
                }
		
                struct stat st;
                stat(name, &st);
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
match_file(int conn) {
	
	LIST_INIT(&head_download);
	
	recv_shared_files(conn);
	filter_and_send(conn);
}

void *
recv_thread(void * param) {
	int conn = (*(int *)param);
	
	LIST_INIT(&head_filestamp);
	
	struct msg header;      
        while(recv_data(conn, (char *)&header, sizeof(struct msg))) {
                // recv broadcasted msg from a client
                char * filename = 0x0;
                char * file = 0x0;
                filename = malloc(header.filenamelen + 1);
                if(header.option != DELETE) 
                        file = malloc(header.state.st_size);
                
                recv_data(conn, filename, header.filenamelen);
		filename[header.filenamelen] = 0x0;
                if(header.option != DELETE) 
                        recv_data(conn, file, header.state.st_size);            
                
		printf("client: recv_thread got broadcasted msg. file : %s, option : %d\n", filename, header.option);
		// modify a file as msg states
		int fd;  				
		if(header.option == DELETE) { // no file stamp on delete. In inotify,check whethter file exist or not
			fd = open(filename, O_RDONLY);
			if(fd != -1) {
				flock(fd, LOCK_EX);
				remove(filename);
			}
		}
		else if(header.option == WRITE) {
			fd = open(filename, O_WRONLY | O_CREAT, header.state.st_mode);
			flock(fd, LOCK_EX);

			unsigned long long file_len = header.state.st_size;
			ssize_t s;
			char * data = file;
			while(file_len > 0 && (s = write(fd, data, file_len)) > 0) {
				file_len -= s;
				data += s;
			}
			// not yet rename
			
                	struct stat st;
                	stat(filename, &st);

			if(!is_exist_client(filename)) {	
				struct file_stamp * n;
				n = (struct file_stamp *)malloc(sizeof(struct file_stamp));
				n->filename = filename;
				n->timestamp = st.st_mtim;
				LIST_INSERT_HEAD(&head_filestamp, n, entries_file_stamp); 	
			}
			else {
				struct file_stamp * np;
				LIST_FOREACH(np, &head_filestamp, entries_file_stamp)
					if(strcmp(np->filename, filename) == 0) {
						np->timestamp = st.st_mtim;
					} 
			}

			free(file);
		}
	
		
		if(fd != -1)
			flock(fd, LOCK_UN);		
	}
		
	printf("client stop(server shutdown)\n");
	clear_list();
	exit(0);
	
}

void *
inotify_thread(void * param) {
	
	int fd;
	int conn = (*(int *)param);

        fd = inotify_init1(0);
        if(fd == -1) {
                perror("inotify_init1");
                exit(1);
        }

        int wd;
        wd = inotify_add_watch(fd, "./", IN_CLOSE_WRITE | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE);
        if(wd == -1) {
                perror("inotify_add_watch");
                exit(1);
        }

	while(1) {
                ssize_t len, i = 0;
                char buf[BUF_LEN] __attribute__((aligned(4)));

                len = read(fd, buf, BUF_LEN);

                while(i < len) {
                        struct inotify_event * event = (struct inotify_event *) &buf[i];
			
			if(event->name[0] == '.') {
				i += sizeof(struct inotify_event) + event->len;
               			 continue;
			}
	
			printf("cleint : inotify_thread detected! ");
			if(event->mask & IN_CLOSE_WRITE)
                                 printf("IN_CLOSE_WRITE");
                        if(event->mask & IN_CREATE)
                                  printf("IN_CREATE");
                        if(event->mask & IN_DELETE)
                             printf("IN_DELETE");
                        if(event->mask & IN_MOVED_FROM)
                                 printf("IN_MOVED_FROM");
                        if(event->mask & IN_MOVED_TO)
                                 printf("IN_MOVED_TO");
                        printf(" , %s\n",event->name);	

			if(event->mask & IN_MOVED_TO || event->mask & IN_CLOSE_WRITE) {
				int file_d = open(event->name, O_RDONLY);
				int found = 0;				

				flock(file_d, LOCK_EX);
				struct timespec timestamp;
				struct file_stamp * np;
				LIST_FOREACH(np, &head_filestamp, entries_file_stamp)
					if(strcmp(np->filename, event->name) == 0) {
						found = 1;
						timestamp = np->timestamp;
						struct stat st;
                				stat(event->name, &st);

						if(st.st_mtim.tv_sec != timestamp.tv_sec) {
							pthread_mutex_lock(&mutex);

							struct entry_send * n;
       							n = (struct entry_send *)malloc(sizeof(struct entry_send));
       							n->fd = file_d;
							n->filename = strdup(event->name);
							n->option = WRITE;
       							LIST_INSERT_HEAD(&head_send, n, entries_entry_send);
					
							pthread_mutex_unlock(&mutex);
						}	
					
					//flock(file_d, LOCK_UN);
				
			    	}

				if(found == 0) {
					pthread_mutex_lock(&mutex);

					struct entry_send * n;
       					n = (struct entry_send *)malloc(sizeof(struct entry_send));
       					n->fd = file_d;
					n->filename = strdup(event->name);
					n->option = WRITE;
     					LIST_INSERT_HEAD(&head_send, n, entries_entry_send);
			
					pthread_mutex_unlock(&mutex);
				
				} 	
			}
			else if(event->mask & IN_MOVED_FROM || event->mask & IN_DELETE) {
				pthread_mutex_lock(&mutex);

				struct entry_send * n;
       				n = (struct entry_send *)malloc(sizeof(struct entry_send));
				n->filename = strdup(event->name);
				n->option = DELETE;
       				LIST_INSERT_HEAD(&head_send, n, entries_entry_send);

				pthread_mutex_unlock(&mutex);
			}
			
                        i += sizeof(struct inotify_event) + event->len;
                }
	}
}

void *
send_thread(void * param) {
	
	int conn = (*(int *)param);

	while(1) {
		pthread_mutex_lock(&mutex);
		
		if(LIST_EMPTY(&head_send)) {
			pthread_mutex_unlock(&mutex);
			continue;
		}

		struct entry_send * n1,* n2;
		struct msg header;

		n1 = LIST_FIRST(&head_send);
                while (1) {
               		n2 = LIST_NEXT(n1, entries_entry_send);
     			if(n2 == 0x0) {
				break;
			}
              		n1 = n2;
           	}

		printf("send_thread(debug). file %s, option %d\n", n1->filename, n1->option);

		if(n1->option == WRITE) {
			header.option = WRITE;
			
			int fd = n1->fd;
			struct stat st;
                        stat(n1->filename, &st);
			header.state = st;
			header.filenamelen = (unsigned char)strlen(n1->filename);

			send_data(conn, &header, sizeof(struct msg));
			send_data(conn, n1->filename, strlen(n1->filename));
			char buf;
			ssize_t read_bites;
                	while((read_bites = read(fd, &buf, 1)) > 0) {
                        	send_data(conn, (void *)&buf, read_bites);
                	}
			close(fd);		
			flock(fd, LOCK_UN);
		}
		else if(n1->option == DELETE) {
			header.option = DELETE;
			header.filenamelen = (unsigned char)strlen(n1->filename);
			send_data(conn, &header, sizeof(struct msg));
			send_data(conn, n1->filename, strlen(n1->filename));
		} 			
		
		LIST_REMOVE(n1, entries_entry_send);
		pthread_mutex_unlock(&mutex);
	}
}

void
client(int sock_fd) {
	match_file(sock_fd);

	pthread_t id_t1, id_t2, id_t3;
	
	pthread_create(&id_t1, NULL, recv_thread, (void *)(&sock_fd));
	pthread_create(&id_t2, NULL, inotify_thread, (void *)(&sock_fd));
	pthread_create(&id_t3, NULL, send_thread, (void *)(&sock_fd));

	/*
	int fd;
        fd = inotify_init1(0);
        if(fd == -1) {
                perror("inotify_init1");
                exit(1);
        }

        int wd;
        wd = inotify_add_watch(fd, "./", IN_CLOSE_WRITE | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE);
        if(wd == -1) {
                perror("inotify_add_watch");
                exit(1);
        }

	while(1) {
                ssize_t len, i = 0;
                char buf[BUF_LEN] __attribute__((aligned(4)));

                len = read(fd, buf, BUF_LEN);

                while(i < len) {
                        struct inotify_event * event = (struct inotify_event *) &buf[i];
			
			if(event->name[0] == '.') {
				i += sizeof(struct inotify_event) + event->len;
               			 continue;
			}

			if(event->mask & IN_CLOSE_WRITE)
                                 printf("IN_CLOSE_WRITE:");
                        if(event->mask & IN_CREATE)
                                  printf("IN_CREATE:");
                        if(event->mask & IN_DELETE)
                             printf("IN_DELETE:");
                        if(event->mask & IN_MOVED_FROM)
                                 printf("IN_MOVED_FROM:");
                        if(event->mask & IN_MOVED_TO)
                                 printf("IN_MOVED_TO:");
                        printf(" %s\n",event->name);	

			if(event->mask & IN_MOVED_TO || event->mask & IN_CLOSE_WRITE) {
				int file_d = open(event->name, O_RDONLY);
				
				flock(file_d, LOCK_EX);
				struct timespec timestamp;
				struct file_stamp * np;
				LIST_FOREACH(np, &head_filestamp, entries_file_stamp)
					if(strcmp(np->filename, event->name) == 0) {
						timestamp = np->timestamp;
						struct stat st;
                				stat(event->name, &st);

						if(st.st_mtim.tv_sec != timestamp.tv_sec) {
							pthread_mutex_lock(&mutex);

							struct entry_send * n;
       							n = (struct entry_send *)malloc(sizeof(struct entry_send));
       							n->fd = file_d;
							n->filename = strdup(event->name);
							n->option = WRITE;
       							LIST_INSERT_HEAD(&head_send, n, entries_entry_send);
					
							pthread_mutex_unlock(&mutex);
						}	
					
					//flock(file_d, LOCK_UN);
				
			    	} 	
			}
			else if(event->mask & IN_MOVED_FROM || event->mask & IN_DELETE) {
				pthread_mutex_lock(&mutex);

				struct entry_send * n;
       				n = (struct entry_send *)malloc(sizeof(struct entry_send));
				n->filename = strdup(event->name);
				n->option = DELETE;
       				LIST_INSERT_HEAD(&head_send, n, entries_entry_send);

				pthread_mutex_unlock(&mutex);
			}
			
                        i += sizeof(struct inotify_event) + event->len;
                }
	}*/

	pthread_join(id_t1, NULL);
	pthread_join(id_t2, NULL);
	pthread_join(id_t3, NULL);
}

int
main(int argc, char * argv[]) {
	
	int sock_fd;
	struct sockaddr_in serv_addr;
	ssize_t addrlen = sizeof(serv_addr);
	
	command_line_interface(argc, argv);

	if(strcmp(mode, "start") == 0) {
		pid_t pid = fork();
		if(pid == 0) {
			signal(SIGTERM, sigterm_handler);
			chdir(shared_directory);
			socket_and_connect(&sock_fd, &serv_addr, addrlen); 
			client(sock_fd);
		}
		else if(pid > 0) {
			char name[256];
			char pid_str[256];
			sprintf(name, "%d", port_num);
			sprintf(pid_str, "%d", pid);
			strcat(name, ".txt");

			int fd = open(name, O_RDWR|O_CREAT, S_IRWXU); // store pid in port_num.txt
			ssize_t len = strlen(pid_str);
			ssize_t s;
			char * data = pid_str;
			while(len > 0 && (s = write(fd, data, len))) {
				len -= s;
				data += s;
			}
				
		char ip_str[17];
      		for(int i = 0; i < 16; i++) {
                	ip_str[i] = ip[i];
       		 }
       		ip_str[16] = 0x0;
		printf("fshare_client start(ip:%s, portnum: %d)\n", ip_str, port_num);
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

		char ip_str[17];
      		for(int i = 0; i < 16; i++) {
                	ip_str[i] = ip[i];
       		 }
       		ip_str[16] = 0x0;
	}

	return 0;
		
}

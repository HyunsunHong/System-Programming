#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <features.h>
#include <netinet/tcp.h>

struct client{
	int conn;
	struct client * next;
};

struct msg_head_t {
	time_t msg_time;
	unsigned char name_len;
	unsigned int msg_len;
};

typedef struct client client;
typedef struct client * client_ptr;
typedef struct msg_head_t msg_head_t;
typedef struct msg_head_t * msg_head_ptr;

client_ptr client_list_head;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int port_num;

void
client_list_print() {

	client_ptr ptr = client_list_head->next;
	int idx = 1;

	while(ptr != 0x0) {
		printf("%d's client conn : %d\n", idx++, ptr->conn);
		ptr = ptr->next;
	}
}

int
recv_a_msg_len(unsigned int * msg_len, int conn) {
	
	ssize_t p = 0;

	for(int p = 0; p < sizeof(unsigned int);) {
		ssize_t r;
		
		r = recv(conn, (char*)msg_len + p, sizeof(unsigned int) - p, 0);
		if(r == 0) // connection failed
			return 0;

		p += r;
	}
	return 1;
}

void *
thread_server(void * param) {
	
	int conn = *((int*)param);
	unsigned char usernamelen;
	char username[9];	
	unsigned int msg_len;
	char msg[4097];
	
	char * data = 0x0;
	unsigned int len = 1;
	int s;
	
	// get user name
	while(len > 0 && (s = recv(conn, &usernamelen, 1, 0)) > 0) {
		len -= s;
	}
	//printf("(debug) usernamelen is %d\n", usernamelen);
	len = usernamelen;
	data = username;	
	while(len > 0 && (s = recv(conn, data, len, 0)) > 0) {
		len -= s;
		data += s;
	}
	username[usernamelen] = 0x0;	
	//printf("(debug) username is  %s\n", username);

	// register client
	pthread_mutex_lock(&mutex);
	client_ptr ptr = client_list_head;
	while(ptr->next != 0x0) 
		ptr = ptr->next;
	client c;
	c.conn = conn;
	c.next = 0x0;
	ptr->next = &c;
	pthread_mutex_unlock(&mutex);
	
	while(recv_a_msg_len(&msg_len, conn)) {
	//while ((s = recv(conn, &msg_len, sizeof(msg_len), 0)) > 0 ) {
		//printf("(debug) msg_len is %d \n",  msg_len);

		data = msg;
		len = msg_len; 

		while(len > 0 && (s = recv(conn, data, len, 0)) > 0) {
			len -= s;
			data += s;
		}	
		//msg[msg_len] = 0x0;
		//printf("(debug) msg is %s \n",  msg);
		
		// broadcast
		pthread_mutex_lock(&mutex);
		time_t curTime;
	       	time(&curTime);	
		msg_head_t msg_head;
		msg_head.msg_time = curTime;
		msg_head.name_len = usernamelen;
		msg_head.msg_len = msg_len;

		client_ptr ptr = client_list_head->next;
		while(ptr != 0x0) {
			int client = ptr->conn;
			
			len = sizeof(msg_head);
			data = (char*)&msg_head;
			while(len > 0 && (s = send(client, data , len , 0)) > 0) {
				len -= s;
				data += s;
			}
	
			/*
			len = sizeof(time_t);
			data =(void *)&curTime;
		        while(len > 0 && (s = send(client, data, len, 0)) > 0) {
				len -= s;
				data += s;
			}
			
			len = sizeof(unsigned char);
			data = (void *)&usernamelen;
			while(len > 0 && (s = send(client, data, len, 0)) > 0) {
				len -= s;
				data += s;
			}*/

			len = usernamelen;
			data = username;
			while(len > 0 && (s = send(client, data, len, 0)) > 0) {
				len -= s;
				data += s;
			}
		
			/*
			len = sizeof(unsigned int);
			data = (void *)&msg_len;
			while(len > 0 && (s = send(client, data, len, 0)) > 0) {
				len -= s;
				data += s;
			}*/

			len = msg_len;
			data = msg;
			while(len > 0 && (s = send(client, data, len, 0)) > 0) {
				len -= s;
				data += s;
			}

			ptr = ptr->next;
		}
		pthread_mutex_unlock(&mutex);
		
	
	}

	
	// When client's server is killed, it escapes above while loop.
	// Reaching here means client is killed.
	printf("%s quit.\n", username);
	// unregister client
	pthread_mutex_lock(&mutex);
	ptr = client_list_head;
	while(ptr->next->conn != conn) {	
		ptr = ptr->next;
	}
	ptr->next = ptr->next->next;
	pthread_mutex_unlock(&mutex);
	
	free(param);

	pthread_exit(0);
}

void 
socket_bind(int * listen_fd, struct sockaddr_in * address, ssize_t addrlen) {

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
server_listen(int listen_fd, struct sockaddr_in * address, ssize_t addrlen) {
        
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
		int opt_val = 1;
        		if(setsockopt(*new_socket, SOL_TCP, TCP_NODELAY, (const char *)&opt_val, sizeof(opt_val)) == -1){
              		 	 perror("setsockopt failed : ");
               			 exit(EXIT_FAILURE);
       		 }

                pthread_t tid;
                pthread_create(&tid, 0x0, thread_server, (void *)new_socket);
        }
}

void command_line_argument(int argc, char const * argv[]) {
	if(argc < 2) {
		printf("Invalid command line interface\n");
		exit(1);
	}
	port_num = atoi(argv[1]);
}

int 
main(int argc, char const *argv[]) 
{ 
	int listen_fd; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 
	
	client c;
	c.conn = 0;
	c.next = 0x0;
	client_list_head = &c; 

	command_line_argument(argc, argv);

	socket_bind(&listen_fd, &address, addrlen);
	
	server_listen(listen_fd, &address, addrlen);
} 


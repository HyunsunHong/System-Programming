#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <features.h>
#include <pthread.h>
#include <time.h>
#include <ncurses.h>

char * ip;
int port_num;
char user_id;
char opt;

struct msg_head_t {
	time_t msg_time;
	unsigned char name_len;
	unsigned int msg_len;
};

typedef struct msg_head_t msg_head_t;
typedef struct msg_head_t * msg_head_ptr;

void * 
thread_send(void * param) {
	int conn = (*(int*)param);
	ssize_t len;
	ssize_t s;
	char * data;
	while(1) {
		char msg[4097];
		unsigned int msg_len; 
		//printf("Enter the msg: "); 
		//fgets(msg, 4097, stdin);
		//msg[strlen(msg) - 1] = 0x0;

		msg_len = strlen(msg);
		len = sizeof(unsigned int);
		data = (char *)&msg_len;
		while(len > 0 && (s = send(conn, data, len, 0)) > 0 ) {
			len -= s;
			data += s;
		}

		data = msg;
		len = strlen(msg);
		while(len > 0 && (s = send(conn, data, len, 0)) > 0 ) {
			data += s;
			len -= s;	
		}
		
		if(s == 0){ 
			pthread_exit(0);
		}
	}	
	
}

int
recv_a_msg_head (msg_head_ptr msg_head, int conn)
{
	ssize_t p = 0 ;

	for (p = 0 ; p < sizeof(msg_head_t) ; ) {
		ssize_t r ;
		
		r = recv(conn, (char*)msg_head + p, sizeof(msg_head_t) - p, 0) ;
		if(r == 0) // connection failed 
			return 0;

		p += r;
	}
	return 1;
}


void *
thread_recv(void * param) {
       	int conn = *((int *)param);
	int first = 1;
	int line = 0;

       	time_t msg_time;
	unsigned char usernamelen;
	char username[9];
	msg_head_t msg_head;
	unsigned int msg_len;
	char msg[4097];

	void * data = 0x0;
	unsigned int len;
	int s;
    
	while (recv_a_msg_head(&msg_head, conn)) {
	//while((s = recv(conn, &msg_time, sizeof(msg_time), 0)) > 0) {
		//recv_a_msg_text(msg_head.length, msg) ;
		
		msg_time = msg_head.msg_time;
		usernamelen = msg_head.name_len;
		msg_len = msg_head.msg_len;	
		
		/*
		len = sizeof(unsigned char);
		while(len > 0 && (s = recv(conn, &usernamelen, len, 0)) > 0) {
        	    len -= s;
 	        }*/
			
		len = usernamelen;
		data = username;
		while(len > 0 && (s = recv(conn, data, len, 0)) > 0) {
        	    len -= s;
		    data += s;
 	        }

		//recv(conn, &msg_len, sizeof(msg_len), 0);

		len = msg_len;
		data = msg;
		while(len > 0 && (s = recv(conn, data, len, 0)) > 0) {
        	    len -= s;
		    data += s;
 	        }
		
		username[usernamelen] = 0x0;
		msg[msg_len] = 0x0;


	}
	
	pthread_exit(0);
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
	int conn = *sock_fd;
	int opt_val = 1;
	if(setsockopt(conn, SOL_TCP, TCP_NODELAY, (const char *)&opt_val, sizeof(opt_val)) == -1){
        	perror("setsockopt failed : ");
        	exit(EXIT_FAILURE);
	}
	
	char name = '4';

	do {
		printf("Enter the user name: ");
		fscanf(stdin, "%c", &name);
	} while (!('0' <= name && name <= '3'));
	// assert name 0 ~ 3

	for (ssize_t s = 0 ; s < 1; ) {
		s += send(conn, &name, 1, 0) ; 
	}
       	
	pthread_t recv_tid, send_tid;
        pthread_attr_t attr1, attr2;

        pthread_create(&recv_tid, NULL, thread_recv, (void *)sock_fd);
        pthread_create(&send_tid, NULL, thread_send, (void *)sock_fd);
	
	pthread_join(recv_tid, NULL);
	pthread_join(send_tid, NULL);
	
}

void
command_line_interface(int argc, char * argv[]) {
	
	if(argc != 3) {
		printf("Command line argument error\n");
		exit(1);
	}
        char * token;
        token  = strtok(argv[1], ":");
        ip = token;
        token = strtok(NULL, " ");
	if(token == NULL) {
		printf("Command line argument error\n");
		exit(1);
	}
        port_num = atoi(token);
	
	if(strlen(argv[2]) > 1) {
		printf("Command line argument error\n");
		exit(1);
	}
	if((argv[2][0] < '0') || (argv[2][0] > '3')) {
		printf("Command line argument error\n");
		exit(1);
	}
	user_id = *(argv[2]);
}


int 
main(int argc, char * argv[]) {
	
	int sock_fd;
	struct sockaddr_in serv_addr;
	ssize_t addrlen = sizeof(serv_addr);

	command_line_interface(argc, argv);
	
	client(&sock_fd, &serv_addr, addrlen); 
	
	return 0;
		
}

#include <stdio.h>
#include <sys/inotify.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define BUF_LEN 1000

void *
inotify_thread(void * param) {
	
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
		char buf[BUF_LEN];
		
		len = read(fd, buf, BUF_LEN);
		
		while(i < len) {
			struct inotify_event * event = (struct inotify_event *) &buf[i];
			//printf("wd = %d mask = %d cookie = %d len = %d\n", event->wd, event->mask, event->cookie, event->len);
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
				
			i += sizeof(struct inotify_event) + event->len;
		}
	}
}

int main() {
	
	//pthread_t id;
	
	//pthread_create(&id, NULL, inotify_thread, 0x0);

        //pthread_join(id, NULL);
	int fd;
	fd = inotify_init1(IN_CLOEXEC);
	if(fd == -1) {
		perror("inotify_init1");
		exit(1);
	}
	
	int wd; 
	wd = inotify_add_watch(fd, ".", IN_CLOSE_WRITE | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE | IN_MOVE_SELF);
	if(wd == -1) {
		perror("inotify_add_watch");
		exit(1);
	}

	while(1) {
		ssize_t len, i = 0;
		char buf[BUF_LEN];
		
		len = read(fd, buf, BUF_LEN);
		
		while(i < len) {
			struct inotify_event * event = (struct inotify_event *) &buf[i];
			//printf("wd = %d mask = %d cookie = %d len = %d\n", event->wd, event->mask, event->cookie, event->len);
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
                        if(event->mask & IN_MOVE_SELF)
                                 printf("IN_MOVED_SELF:");

                        printf(" %s\n",event->name);	
				
			i += sizeof(struct inotify_event) + event->len;
		}
	}


	
	return 0;
}

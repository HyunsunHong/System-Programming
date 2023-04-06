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

#define BUF_LEN 1000

int main() {

	int fd;
        //int conn = (*(int *)param);

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
                       	/*
			 if(event->mask & 128 || event->mask & 8) {
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

                                        flock(file_d, LOCK_UN);

                                }
                        }
                        else if(event->mask & 64 || event->mask & 512) {
                                pthread_mutex_lock(&mutex);

                                struct entry_send * n;
                                n = (struct entry_send *)malloc(sizeof(struct entry_send));
                                n->filename = strdup(event->name);
                                n->option = DELETE;
                                LIST_INSERT_HEAD(&head_send, n, entries_entry_send);

                                pthread_mutex_unlock(&mutex);
                        }*/

                        i += sizeof(struct inotify_event) + event->len;
                }
        }

	return 0;
}

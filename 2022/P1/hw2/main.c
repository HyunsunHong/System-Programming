#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>

char * testdir = NULL;
char * solution = NULL;
char * target = NULL;
int timeout = 0;
double min_time = 0;
double max_time = 0;
double sum_time = 0;
double duration = 0;

void help() {
	printf("Coomand line arguments should be typed as below\n");
	printf("$ pctest -i <testdir> -t <timeout> <solution> <target>\n");
	printf("<testdir>: absolute directory that contains only input txt files\n");
	printf("<timeout>: 1 to 10 integer value\n");
	printf("<solution>: absolute directory for solution.c file\n");
	printf("<target>: absolute directory for target.c file\n");
}

char * str_concate(char * origin, char * append) {
	size_t s = strlen(origin) + strlen(append) + 1;
	char * str_concated = (char*)calloc(s, sizeof(char));
	strcat(str_concated, origin);
	strcat(str_concated, append);

	return str_concated;
}


void *timer(void *param) {
	struct timespec begin, end;
	
	duration = 0;
	clock_gettime(CLOCK_MONOTONIC, &begin);

	while(duaration < timeout * 1000000) {
		clock_gettime(CLOCK_MONOTONIC, &end);

		long time = (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec);

		duaratin = (double)time/1000000;
	}
	
	kill(pid, SIGKILL);
	pthread_exit(0);	
}


int main(int argc, char *argv[]) {

	int flag_i = 0, flag_t = 0;
	int opt;
	int fd;
	int gcc_pipes[2];
	int compile_success = 1;
	int tests_cnt = -2;
	DIR * dp = 0x0;
	struct dirent * dir_entry = 0x0;
	struct stat tmp_stat;

	pid_t pid;

	// Command-line interface
	// Check validity of command-line usage
	while((opt = getopt(argc, argv, "i:t:")) != -1) {
		switch(opt) {
			case 'i':
				flag_i = 1;
				testdir = optarg;
				break;
			case 't':
				flag_t = 1;
				timeout = atoi(optarg);
				if(timeout < 1 || timeout > 10) {
					help();
					exit(EXIT_FAILURE);
				}
				break;
			case '?':
				help();
				exit(EXIT_FAILURE);
		}
	}
	if(flag_i == 0 || flag_t == 0 || argc < 7) {
		help();
		exit(EXIT_FAILURE);
	}
	solution = argv[5];
	target = argv[6];
	
	// Check validity of <testdir>, <solution> and <target> 
	errno = 0;
	if((dp = opendir(testdir)) == NULL) {
		switch(errno) {
			case EACCES: printf("<testdir>: permission denied\n"); break;
			case ENOENT: printf("<testdir>: directory does not exist\n"); break;
			case ENOTDIR: printf("<testdir>: '%s' is not a directory\n", argv[2]); break;
		}
		exit(EXIT_FAILURE);
	}
	while(dir_entry = readdir(dp)) {
		tests_cnt++;
	} 
	if(tests_cnt == 0) {
		printf("<testdir>: There is no file in '%s'\n", argv[2]); 
		exit(EXIT_FAILURE);
	}
	
	if(closedir(dp) == -1) {
		perror("closedir");
		exit(EXIT_FAILURE);
	}
	
	if(lstat(solution, &tmp_stat) == -1) {
		perror("<solution>");
   		exit(EXIT_FAILURE);
	}

	if(lstat(target, &tmp_stat) == -1) {
		perror("<target>");
		exit(EXIT_FAILURE);
	}
	
	
	// Build two executable files
	// Compile with GCC
	pid = fork();
	if(pid < 0) { 				// solution.c compile
		printf("Fork failed\n");
		exit(EXIT_FAILURE);
	}
	else if(pid == 0) {
		execlp("gcc", "gcc", solution, "-o", "solution" , NULL);
	}
	else {
		wait(NULL);
	}
	
	if(pipe(gcc_pipes) != 0) {           	// target.c compile
		perror("gcc_pipe");
		exit(EXIT_FAILURE);
	}
       	pid = fork();
        if(pid < 0) {
                printf("Fork failed\n");
                exit(EXIT_FAILURE);
        }
        else if(pid == 0) {
		close(gcc_pipes[0]);
		dup2(gcc_pipes[1], STDERR_FILENO);
                execlp("gcc", "gcc", target, "-static" ,"-o", "target" , NULL);
        }
        else {
	      	int status;	
                wait(&status);

		if(status != 0) {
			compile_success = 0;	
		}
	} // If compile error occured, its error msg is stored in the gcc_pipes
	close(gcc_pipes[1]);	
	close(gcc_pipes[0]);	
	if(compile_success == 0) {
		printf("Target source file compilation failed\n");
		exit(EXIT_FAILURE);
	}

	// Execute solution and target files	
	dp = opendir(testdir);

	while(dir_entry = readdir(dp)) {
		if(strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0) continue;

		char * test_file = str_concate(testdir, dir_entry->d_name);
		int fd;
		int solution_pipe[2];
		int target_pipe[2];
		char buf[32];
		ssize_t s;

		printf("%s\n", test_file);
	       	//printf("fd of DIR: %d\n", dirfd(dp));

		if(pipe(solution_pipe) != 0) {  		// solution execution
                        perror("solution_pipe");
                       	free(test_file);
		       	exit(EXIT_FAILURE);
                }
		if((fd = open(test_file, O_RDONLY)) < 0) {
                         perror("file open error");
                         free(test_file);
                         exit(EXIT_FAILURE);
                }
		//printf("solution pipe: %d %d\n", solution_pipe[0], solution_pipe[1]);
		//printf("first fd: %d\n", fd);
		dup2(fd, STDIN_FILENO);
		pid = fork();
        	if(pid < 0) {
               		 printf("Fork failed\n");
			 free(test_file);
               		 exit(EXIT_FAILURE);
       		 }
        	else if(pid == 0) {
               		close(solution_pipe[0]);
                	dup2(solution_pipe[1], STDOUT_FILENO);
			execlp("./solution", "solution", NULL);
        	}
        	else {
                	int status;
                	wait(&status);

			close(fd);			
			
			/*
			close(solution_pipe[1]);
			while((s = read(solution_pipe[0], buf, 31)) > 0) {
				buf[s] = 0x0;
				printf("%s", buf);
			}
			*/
			
		}
		

   		if(pipe(target_pipe) != 0) {                  // target execution
                        perror("target_pipe");
                        free(test_file);
                        exit(EXIT_FAILURE);
                }
		if((fd = open(test_file, O_RDONLY)) < 0) {
                         perror("file open error");
                         free(test_file);
                         exit(EXIT_FAILURE);
                }
		//printf("target pipe: %d %d\n", target_pipe[0], target_pipe[1]);
		//printf("second fd: %d\n", fd);
                dup2(fd, STDIN_FILENO);
		pid = fork();                        
                if(pid < 0) {
                         printf("Fork failed\n");
			 free(test_file);
                         exit(EXIT_FAILURE);
                 }
                else if(pid == 0) {
                        close(target_pipe[0]);
                        dup2(target_pipe[1], STDOUT_FILENO);
			
			struct rlimit rlim;
			int ret;

			ret = getrlimit(RLIMIT_NOFILE, &rlim);
			if(ret == -1) {
				perror("getlimit");
			 	free(test_file);
				exit(EXIT_FAILURE);
			}

			rlim.rlim_cur = 4;
			ret = setrlimit(RLIMIT_NOFILE, &rlim);
			if(ret == -1) {
				perror("setrlimit");
			 	free(test_file);
				exit(EXIT_FAILURE);
			}

                        execlp("./target", "target", NULL);
                }
                else {
			int status;
			pthread_t tid;
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			
			pthread_create(&tid, &attr, timer, pid);
			wait(&status);

			close(fd);
			
			pthread_cancel(tid);

			if(duration >= timeout * 1000000) 
				printf("Timeover\n");
			else {
				if(min_time == 0 || min_time > duration) 
					min_time = duration;
				if(max_time == 0 || max_time < duaration)
					max_time = duration;

				sum_time += duaration;
			}

			/*
		 	close(target_pipe[1]);
 			while((s = read(target_pipe[0], buf, 31)) > 0) {
                                buf[s] = 0x0;
                                printf("%s", buf);
                        }
			*/
			
			
			if(status != 0 && duration < timeout * 1000000) {
				printf("Child termination error\n");
			 	free(test_file);
				exit(EXIT_FAILURE);
			}
                } 
		// 1.Output of solution and target are stored in each pipes (output compare)
		// 2.Error code termination is detected through wait and status value (check status value)
		// 3.Timeout check thorugh thread, and kill 
		
		close(solution_pipe[0]);
		close(solution_pipe[1]);
		close(target_pipe[0]);
		close(target_pipe[1]);

		free(test_file);
	}

	closedir(dp);

	exit(EXIT_SUCCESS);
}

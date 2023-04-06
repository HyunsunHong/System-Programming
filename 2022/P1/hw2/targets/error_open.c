#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>

int main(int argc, char* argv[]){

	char local[5] = "..";

	//DIR* d = opendir(local);
	//struct dirent *rd = readdir(d);

	//char *s = rd->d_name;
	//printf("%s\n", s);
	//
   	int fd = open("/home/hyunsun/summer_study/OS/hw2/Tests/test1.txt", O_RDWR|O_CREAT, 755);

	printf("\n\n\nfd is %d\n", fd);	
	

	int a, b;
	int add = 0, sub = 0, mul = 0, div = 0;

	scanf("%d %d", &a, &b);

	add = a + b;
	sub = a - b;
	mul = a * b;
	div = a / b;

	printf("a is %d b is %d\n result:\naddtion = %d\nsubtraction = %d\nmultiplication = %d\ndivision = %d\n", a, b, add, sub, mul, div);

	return 0;
}

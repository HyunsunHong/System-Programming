#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>

int main() {

	void * p1 = mmap(0x0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON, -1, 0);
	void * p2 = mmap(0x0, getpagesize()*3, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON, -1, 0);
	
	printf("first allocated %p\n", p1);
	printf("second allocated %p\n", p2);

	if(munmap(p1, getpagesize()) != 0) {
		//perror();
		printf("worn:");
	}	
	/*
	if(munmap(p2, getpagesize() + 1) != 0) {
		printf("faile\n");
	}*/
	
	printf("%p\n", p2 + getpagesize()/2);	
	char * a = (char *)(p2 + getpagesize()/2);
	(*a) = 'c';

	printf("%c\n",(*a));

	return 0;
}

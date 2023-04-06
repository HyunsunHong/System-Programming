#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]){
	int a, b;
	int add = 0, sub = 0, mul = 0, div = 0;

	scanf("%d %d", &a, &b);

	add = a + b;
	sub = a - b;
	mul = a * b;
	div = a / b;

	printf("a is %d b is %d\n result:\naddtion = %d\nsubtraction = %d\nmultiplication = %d\ndivision = %d\n", a, b, add, sub, mul, div);
	
	exit(1);
}



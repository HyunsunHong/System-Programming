#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]){
	int a, b;
	int add = 0, sub = 0, mul = 0, div = 0;

	scanf("%d %d", &a, &b);
	int c[2];

	printf("%d", c[3]);
	add = a + b;
	sub = a - b;
	mul = a * b;
	printf("before\n");
	div = a / 0;
	printf("after\n");

	printf("a is %d b is %d\n result:\naddtion = %d\nsubtraction = %d\nmultiplication = %d\ndivision = %d\n", a, b, add, sub, mul, div);

	return 0;
}

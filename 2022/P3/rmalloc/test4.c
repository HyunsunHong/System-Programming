#include <stdio.h>
#include "rmalloc.h"

int 
main ()
{
	void *p1, *p2, *p3, *p4 ;

	rmprint() ;
	
	p1 = rmalloc(2000) ; 
	printf("rmalloc(2000):%p\n", p1) ; 
	rmprint() ;
	/*
	// case 1
	p1 = rrealloc(p1, 1990) ; 
	printf("rrealloc(p1, 1990):%p\n", p1) ; 
	rmprint() ;
	
	// case 4	
	p1 = rrealloc(p1, 2010) ; 
	printf("rrealloc(p1, 2010):%p\n", p1) ; 
	rmprint() ;
	
		
	p1 = rrealloc(p1, 2000) ; 
	printf("rrealloc(p1, 2000):%p\n", p1) ; 
	rmprint() ;
	*/	
	
	p2 = rmalloc(1000) ; 
	printf("rmalloc(1000):%p\n", p2) ; 
	rmprint() ;
	
	
	p3 = rmalloc(1000) ; 
	printf("rmalloc(1000):%p\n", p3) ; 
	rmprint() ;

	// case 2
	p2 = rrealloc(p2, 1900) ; 
	printf("rrealloc(p2, 1900):%p\n", p2) ; 
	rmprint() ;
}

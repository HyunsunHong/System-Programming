#include <stdio.h>
#include "smalloc.h"

int 
main()
{
	void *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8 ;

	print_sm_containers() ;

	p1 = smalloc(1500) ; 
	printf("smalloc(1500):%p\n", p1) ; 
	print_sm_containers() ;
	
	// down-sizing case 3
	printf("srealloc(1490):%p\n", p1) ; 
	p1 = srealloc (p1, 1490) ;
	print_sm_containers() ;
	
	// down-sizing case 4
	printf("srealloc(1390):%p\n", p1) ; 
	p1 = srealloc (p1, 1390) ;
	print_sm_containers() ;

	p2 = smalloc(2642) ; 
	printf("smalloc(2642):%p\n", p2) ; 
	print_sm_containers() ;

	// down-sizing case 1
	printf("srealloc(2632):%p\n", p2) ; 
	p2 = srealloc (p2, 2632) ;
	print_sm_containers() ;

	// down-sizing case 2
	printf("srealloc(1380):%p\n", p1) ; 
	p1 = srealloc (p1, 1380) ;
	print_sm_containers() ;

	sshrink();
	printf("sshrink\n") ; 
	print_sm_containers() ;
	
	
	// up-sizing case 1
	printf("srealloc(1390):%p\n", p1) ; 
	p1 = srealloc (p1, 1390) ;
	print_sm_containers() ;
	
	// up-sizing case 2
	printf("srealloc(2642):%p\n", p2) ; 
	p2 = srealloc (p2, 2642) ;
	print_sm_containers() ;

	// up-sizing case 3
	printf("srealloc(1400):%p\n", p1) ; 
	p1 = srealloc (p1, 1400) ;
	print_sm_containers() ;

	// up-sizing case 3(another version)
	printf("srealloc(2652):%p\n", p2) ; 
	p2 = srealloc (p2, 2652) ;
	print_sm_containers() ;
	
	// up-sizing case 5
	printf("srealloc(8000):%p\n", p2) ; 
	p2 = srealloc (p2, 8000) ;
	print_sm_containers() ;
	
	// up-sizing case 4
	printf("srealloc(8500):%p\n", p2) ; 
	p2 = srealloc (p2, 8500) ;
	print_sm_containers() ;

	sfree(p1);
        printf("sfree:%p\n", p1) ;
        print_sm_containers() ;

	sshrink();
	printf("sshrink\n") ; 
	print_sm_containers() ;

}

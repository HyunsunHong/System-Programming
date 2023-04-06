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

	printf("srealloc(1490):%p\n", p1) ; 
	p1 = srealloc (p1, 1490) ;
	print_sm_containers() ;


	p2 = smalloc(2500) ; 
	printf("smalloc(2500):%p\n", p2) ; 
	print_sm_containers() ;

	printf("srealloc(1480):%p\n", p1) ; 
	p1 = srealloc (p1, 1480) ;
	print_sm_containers() ;

	print_mem_uses() ;

	p3 = smalloc(2800) ; 
	printf("smalloc(2800):%p\n", p3) ; 
	print_sm_containers() ;

	p4 = smalloc(1100) ; 
	printf("smalloc(1100):%p\n", p4) ; 
	print_sm_containers() ;
	
	printf("srealloc(2000):%p\n", p4) ; 
	p4 = srealloc (p4, 2000) ;
	print_sm_containers() ;

	print_mem_uses() ;

	p5 = smalloc(3000) ; 
	printf("smalloc(3000):%p\n", p5) ; 
	print_sm_containers() ;

	p6 = smalloc(1200) ; 
	printf("smalloc(1200):%p\n", p6) ; 
	print_sm_containers() ;

	p7 = smalloc(800) ; 
	printf("smalloc(800):%p\n", p7) ; 
	print_sm_containers() ;

	print_mem_uses() ;

	sfree(p2);
	printf("sfree:%p\n", p2) ; 
	print_sm_containers() ;

	printf("srealloc(4000):%p\n", p5) ; 
	p5 = srealloc (p5, 4000) ;
	print_sm_containers() ;

	printf("srealloc(8150):%p\n", p3) ; 
	p3 = srealloc (p3, 8150) ;
	print_sm_containers() ;

	sshrink();
	printf("sshrink\n") ; 
	print_sm_containers() ;

}

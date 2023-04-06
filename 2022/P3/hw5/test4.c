#include <stdio.h>
#include "smalloc.h"

int
main()
{
        void *p1, *p2, *p3, *p4 ;

        print_sm_containers() ;
	
        p1 = smalloc(2000) ;
        printf("smalloc(2000):%p\n", p1) ;
        print_sm_containers() ;

        p2 = smalloc(3000) ;
        printf("smalloc(3000):%p\n", p2) ;
   	print_sm_containers() ;
	
	
        p3 = smalloc(100) ;
        printf("smalloc(100):%p\n", p3) ;
        print_sm_containers() ;

        p4 = smalloc(2032) ;
        printf("smalloc(2032):%p\n", p4) ;
        print_sm_containers() ;
}

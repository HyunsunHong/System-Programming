#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "smalloc.h" 

sm_container_t sm_head = {
	0,
	&sm_head, 
	&sm_head,
	0 
} ;

static 
void * 
_data (sm_container_ptr e)
{
	return ((void *) e) + sizeof(sm_container_t) ;
}

void *
merge(sm_container_ptr curr) {
	sm_container_ptr right = curr -> next;
	sm_container_ptr left = curr -> prev;

	if(/*right != &sm_head &&*/ right -> status == Unused) {
		curr -> dsize += sizeof(sm_container_t) + right->dsize;
		curr -> next = right -> next;
		right -> next -> prev = curr;
	}
	if(/*left != &sm_head &&*/ left -> status == Unused) {
		left -> dsize += sizeof(sm_container_t) + curr -> dsize;
		left -> next = curr -> next;
		curr -> next -> prev = left;
		return left;
	}

	return curr;
}

static 
void 
sm_container_split (sm_container_ptr hole, size_t size)
{
	sm_container_ptr remainder = (sm_container_ptr) (_data(hole) + size) ;

	remainder->dsize = hole->dsize - size - sizeof(sm_container_t) ;	
	remainder->status = Unused ;
	remainder->next = hole->next ;
	remainder->prev = hole ;
	hole->dsize = size ;
	hole->next->prev = remainder ;
	hole->next = remainder ;
}

static 
void * 
retain_more_memory (int size)
{
	sm_container_ptr hole ;
	int pagesize = getpagesize() ;
	int n_pages = 0 ;

	n_pages = (sizeof(sm_container_t) + size + sizeof(sm_container_t)) / pagesize  + 1 ;
	hole = (sm_container_ptr) sbrk(n_pages * pagesize) ;
	if (hole == 0x0)
		return 0x0 ;
	
	hole->status = Unused ;
	hole->dsize = n_pages * getpagesize() - sizeof(sm_container_t) ;
	return hole ;
}

void * 
smalloc (size_t size) 
{
	sm_container_ptr hole = 0x0, itr = 0x0 ;
	size_t curr_size = getpagesize();

	for (itr = sm_head.next ; itr != &sm_head ; itr = itr->next) {
		if (itr->status == Busy)
			continue ;
		if ((itr->dsize == size) || (size + sizeof(sm_container_t) < itr->dsize)) {
			// best fit
			if(curr_size > itr->dsize) {
				curr_size = itr->dsize;	
				hole = itr ; 
			}
			// first fit
			/*
			 hole = itr;
			 break;
			 */
		}
	}
	if (hole == 0x0) {
		hole = retain_more_memory(size) ;
		if (hole == 0x0)
			return 0x0 ;
		hole->next = &sm_head ;
		hole->prev = sm_head.prev ;
		(sm_head.prev)->next = hole ;
		sm_head.prev = hole ;
	}
	if (size < hole->dsize) 
		sm_container_split(hole, size) ;
	hole->status = Busy ;
	return _data(hole) ;
}

void 
sfree (void * p)
{
	sm_container_ptr itr ;
	for (itr = sm_head.next ; itr != &sm_head ; itr = itr -> next) {
		if (p == _data(itr)) {
			itr->status = Unused;
			merge(itr);	
			break ;
		}
	}
}

void *
srealloc(void * p, size_t newsize) {
	sm_container_ptr curr = (sm_container_ptr)(p - sizeof(sm_container_t));
	size_t oldsize = curr -> dsize;

	if(newsize <= 0) {
		sfree(p);
		return 0x0;
	}
	if(newsize == oldsize) {
		return p;
	}

	// donwsize memory
	if(newsize < oldsize) {
		size_t to_shrink = oldsize - newsize;
		
		// (4)
		if(to_shrink > sizeof(sm_container_t)) {
			sm_container_split(curr, newsize);
			merge(curr->next);
			return p; 		
		}
		else {
			// (2)
			if(curr->next != &sm_head && curr->next->status == Busy) {
				curr->status = Unused;
				merge(curr);
				
				void * src = _data(curr);
				void * dest = 0x0;
				ssize_t n = newsize;

				curr = smalloc(newsize);
				dest = (void *)curr;
			       	memmove(dest, src, n);

				return curr;	
			}
			else {
				void * dest = _data(curr) + newsize;
			       	void * src = 0;	
				sm_container_ptr hole;
				
				// (1)
				if(curr->next == &sm_head) {
					hole = retain_more_memory(1);
					if (hole == 0x0)
						return 0x0 ;
					hole->next = &sm_head ;
					hole->prev = sm_head.prev ;
					(sm_head.prev)->next = hole ;
					sm_head.prev = hole ;	

				}
				else { // (3) check
					hole = curr->next;
				}

				curr->next->next->prev = (sm_container_ptr)dest;
				curr->next = (sm_container_ptr)dest;

				hole->dsize += (void *)hole - dest;
				curr->dsize = newsize;

				src =(void *)hole;	

				memmove(dest, src, sizeof(sm_container_t));
			
				return p;
			}

		}
	}

	// upsize memory
	if(newsize > oldsize) {
		size_t to_upsize = newsize - oldsize;
		
		// is nothing?
		if(curr->next->status != Unused) {
			// is open?
			// (1)
			if(curr->next == &sm_head) {
				sm_container_ptr hole;
				hole = retain_more_memory(to_upsize);
				
				curr->dsize += to_upsize;
				hole->dsize -= to_upsize;

				void * src = (void *)hole;
				void * dest = src + to_upsize;

				memmove(dest, src, sizeof(sm_container_t));
				
				sm_head.prev = (sm_container_ptr)dest;
				curr->next = (sm_container_ptr)dest;

				return p;
			} // (2)
			else {
				size_t curr_size = curr->dsize;
				
				void * src = (void *)_data(curr);	
				void * dest = 0x0;

				sfree(_data(curr));
				dest = smalloc(curr_size + to_upsize);

				memmove(dest, src, curr_size);
				
				return dest;
			}
		}
		else {
			// is enough?
			// (3)
			if((curr->next->dsize) > to_upsize) {
				void * src = (void *)(curr->next);
				void * dest = src + to_upsize;
				
				curr->next->dsize -= to_upsize;
				curr->dsize += to_upsize;

				curr->next->next->prev = (sm_container_ptr)((void *)curr->next + to_upsize);
				curr->next = (sm_container_ptr)((void *)curr->next + to_upsize);

				memmove(dest, src, sizeof(sm_container_t));
				
				return p;
			} // again, is open?
			else {
				// (4) 
				if(curr->next->next == &sm_head) {
					
					printf("0\n");
					sm_container_ptr hole = retain_more_memory(to_upsize - curr->next->dsize);
					if (hole == 0x0)
						return 0x0 ;
					hole->next = &sm_head ;
					hole->prev = sm_head.prev ;
					(sm_head.prev)->next = hole ;
					sm_head.prev = hole ;
					
					printf("1\n");
					hole = merge(hole);

					printf("2\n");
					void * src = (void *)(curr->next);
					void * dest = src + to_upsize;

					curr->next->dsize -= to_upsize;
					curr->dsize += to_upsize;

					printf("3\n");
					curr->next->next->prev = (sm_container_ptr)((void *)(curr->next) + to_upsize);
					curr->next = (sm_container_ptr)((void *)(curr->next) + to_upsize);

					printf("4\n");
					memmove(dest, src, sizeof(sm_container_t));
				
					printf("5\n");
					return p;
				} // (5)
				else {
					size_t curr_size = curr->dsize;
				
					void * src = (void *)_data(curr);	
					void * dest = 0x0;

					sfree(_data(curr));
					dest = smalloc(curr_size + to_upsize);

					memmove(dest, src, curr_size);
				
					return dest;
				}
			}
		}
		
	}	
}

void 
print_sm_containers ()
{
	sm_container_ptr itr ;
	int i ;

	printf("==================== sm_containers ====================\n") ;
	for (itr = sm_head.next, i = 0 ; itr != &sm_head ; itr = itr->next, i++) {
		printf("%3d:%p:%s:", i, _data(itr), itr->status == Unused ? "Unused" : "  Busy") ;
		printf("%8d:", (int) itr->dsize) ;

		int j ;
		char * s = (char *) _data(itr) ;
		for (j = 0 ; j < (itr->dsize >= 8 ? 8 : itr->dsize) ; j++) 
			printf("%02x ", s[j]) ;
		printf("\n") ;
	}
	printf("\n") ;

}

void
print_mem_uses() {
	
	sm_container_ptr itr;
	unsigned long retained = 0;
	unsigned long allocated = 0;
	unsigned long not_allocated = 0;

	printf("==================== mem_uses ====================\n");

	for(itr = sm_head.next; itr != &sm_head; itr = itr -> next) {
		retained += itr->dsize + sizeof(sm_container_t);
		if(itr->status == Busy) {
			allocated += itr->dsize;
		}
		else if(itr->status == Unused) {
			not_allocated += itr->dsize;
		}
	}

	printf("%15s%10lu\n", "  Retained mem(bytes)      :", retained);
	printf("%15s%10lu\n", "  Allocated mem(bytes)     :", allocated);
	printf("%15s%10lu\n\n","  Not allocated mem(bytes) :", not_allocated);
}

void sshrink() {
	sm_container_ptr end = sm_head.prev;

	if(end->status == Busy)
		return;

	end->prev->next = &sm_head;
	sm_head.prev = end->prev;

	void * break_point = sbrk(0);
	break_point -= sizeof(sm_container_t) + end->dsize;

	brk(break_point);
}

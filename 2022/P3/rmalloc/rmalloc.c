#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include "rmalloc.h" 
#include <assert.h>
#include <string.h>

rm_header rm_free_list = { 0x0, 0 } ;
rm_header rm_used_list = { 0x0, 0 } ;
rm_option curr_opt = FirstFit;

static
void *
_data(rm_header_ptr e) {
	return ((void *) e) + sizeof(rm_header);
}

static
void 
rm_container_split(rm_header_ptr hole, size_t size) {
	
	rm_header_ptr remainder = (rm_header_ptr) (_data(hole) + size);

	remainder->size = hole->size - size - sizeof(rm_header);
       	remainder->next = hole->next;
	hole->size = size;
	hole->next = remainder;	
}

static 
void *
retain_more_memory(int size) {
	
	rm_header_ptr hole;
	int pagesize = getpagesize();
	int n_pages = 0;

	n_pages = (sizeof(rm_header) + size + sizeof(rm_header)) / pagesize + 1;
	hole = (rm_header_ptr)mmap(0x0, pagesize * n_pages, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON, -1, 0);
	if(hole == 0x0)
		return 0x0;

	hole->next = 0x0;
	hole->size = n_pages * pagesize - sizeof(rm_header);
	return hole;
}	

static
void 
rm_move_to_used(rm_header_ptr to_move) {
	rm_header_ptr itr;

	itr = &rm_free_list;
	while(itr->next != to_move)
		itr = itr->next;
	itr->next = itr->next->next;
	
	itr = &rm_used_list;
	while(itr->next != 0x0) 
		itr = itr->next;
	itr->next = to_move;
	to_move->next = 0x0;
}

void * rmalloc (size_t s) 
{ 
	rm_header_ptr hole = 0x0, itr = 0x0;
	int is_first = 1;
	size_t temp_size;
		
	for(itr = rm_free_list.next; itr != 0x0; itr = itr->next) {
		if( (itr->size == s) || (s + sizeof(rm_header) < itr->size)) {
			if(curr_opt == FirstFit) {
				hole = itr;
				break;
			}
			if(is_first) {
				is_first = 0;
				temp_size = itr->size;
				hole = itr;
				continue;
			}

			
			if((curr_opt == BestFit) && (itr->size < temp_size)) {
				temp_size = itr->size;
				hole = itr;
			}
			if((curr_opt == WorstFit) && (itr->size > temp_size)) {
				temp_size = itr->size;
				hole = itr;
			}			
		}	
	}
	
	if(hole == 0x0) {
		hole = retain_more_memory(s);
		if(hole == 0x0)
			return 0x0;
		itr = &rm_free_list;

		while(itr->next != 0x0)
			itr = itr->next;
		itr->next = hole;
	}
	if(s < hole->size) 
		rm_container_split(hole, s);
	rm_move_to_used(hole);
	return _data(hole);
}
	
void merge(rm_header_ptr to_move) {

	rm_header_ptr to_move_prev = &rm_free_list;
	
	while(to_move_prev->next != to_move)
		to_move_prev = to_move_prev->next;

	rm_header_ptr itr = 0x0;
	itr = &rm_free_list;

	while(itr->next != 0x0) {
		void * itr_sp = (void *)(itr->next);
		void * itr_ep = (void *)(itr->next) + sizeof(rm_header) + itr->next->size - sizeof(char);
		void * to_move_sp = (void *)to_move;
		void * to_move_ep = (void *)to_move + sizeof(rm_header) + to_move->size - sizeof(char);

		if(to_move_ep == itr_sp - sizeof(char)) {
			to_move->size += itr->next->size + sizeof(rm_header);
			itr->next = itr->next->next;
			break;
		}
		else if(itr_ep == to_move_sp - sizeof(char)){
			itr->next->size += to_move->size + sizeof(rm_header);
			to_move_prev->next = to_move_prev->next->next;
			//to_move = itr->next;
			break;
		}
		itr = itr->next;
	}
	
}


void rfree (void * p) 
{	
	assert(p != 0x0);

	rm_header_ptr itr = 0x0;
	rm_header_ptr to_move_prev = &rm_used_list;
	rm_header_ptr to_move = 0x0;

	while(to_move_prev->next != 0x0 && _data(to_move_prev->next) != p)
		to_move_prev = to_move_prev->next;

	assert(to_move_prev->next != 0x0);

	to_move = to_move_prev->next;
	to_move_prev->next = to_move_prev->next->next;
	
	itr = &rm_free_list;

	while(itr->next != 0x0) {
		void * itr_sp = (void *)(itr->next);
		void * itr_ep = (void *)(itr->next) + sizeof(rm_header) + itr->next->size - sizeof(char);
		void * to_move_sp = (void *)to_move;
		void * to_move_ep = (void *)to_move + sizeof(rm_header) + to_move->size - sizeof(char);

		if(to_move_ep == itr_sp - sizeof(char)) {
			to_move->next = itr->next->next;
			to_move->size += itr->next->size + sizeof(rm_header);
			itr->next = to_move;
			break;
		}
		else if(itr_ep == to_move_sp - sizeof(char)){
			itr->next->size += to_move->size + sizeof(rm_header);
			to_move = itr->next;
			break;
		}
		itr = itr->next;
	}

	if(itr->next == 0x0) {
		itr->next = to_move;
		to_move->next = 0x0;
		return;
	}

	merge(to_move); 
}

void * rrealloc (void * p, size_t s) 
{	if(s <= 0) {
		printf("Invalid realloc size\n");
		return 0x0;
	}
	rm_header_ptr target_prev = &rm_used_list;
	rm_header_ptr target = 0x0;

	while((target_prev->next != 0x0) && (_data(target_prev->next) != p))
		target_prev = target_prev->next;
	if(target_prev->next == 0x0) {
		return rmalloc(s);
	}
	target = target_prev->next;
	
	if(target->size == s){
		return _data(target);
	}
	else if(target->size > s) {
		void * free_ep = _data(target) + target->size - sizeof(char);
		rm_header_ptr itr = &rm_free_list;
		size_t to_shrink = target->size - s;

		while((itr->next != 0x0) && ((void *)(itr->next) - sizeof(char) != free_ep)) {
			itr = itr->next;
		}
		
		if(itr->next != 0x0) {
			target->size -= to_shrink;
			itr->next->size += to_shrink;
			memmove((void *)(itr->next) - to_shrink, (void *)(itr->next), sizeof(rm_header));
		       	itr->next = (rm_header_ptr)((void *)(itr->next) - to_shrink);
			return _data(target);	
		}

			printf("hi there!\n");
		if(to_shrink >= (sizeof(rm_header) + sizeof(char))) {
			printf("hi there!\n");
			target->size -= to_shrink;
			itr = &rm_free_list;
			while(itr->next != 0x0)
				itr = itr->next;
			itr->next = (rm_header_ptr)(_data(target) + target->size - to_shrink);
			itr->next->next = 0x0;
			itr->next->size = to_shrink - sizeof(rm_header);
			return _data(target);
		}
		else {
			void * ret = rmalloc(s);
			memmove(ret, _data(target), s);
			rfree(target);
			return ret;
		}	
	}
	else if(target->size < s) {
		void * ep = _data(target) + target->size - sizeof(char);
		rm_header_ptr itr = &rm_free_list;
		size_t to_expand = s - target->size;

		while((itr->next != 0x0) && ((void *)(itr->next) - sizeof(char) != ep)) {
			itr = itr->next;
		}
		
		if(itr->next != 0x0) {
			size_t next_size = sizeof(rm_header) + itr->next->size;
			if(next_size == to_expand) {
				target->size = s;
				itr->next = itr->next->next;
				return _data(target);
			}
			else if(next_size > to_expand + sizeof(rm_header)) {
				target->size = s;
				itr->next->size -= to_expand;
				memmove((void *)(itr->next) + to_expand, (void *)(itr->next), sizeof(rm_header));
				itr->next = (void *)(itr->next) + to_expand;
				return _data(target);
			}
		}
		void * ret = rmalloc(s);
		memmove(ret, _data(target), target->size);
		rfree(target);
		return ret;
	}

}

void rmshrink () 
{
	// TODO
}

void rmconfig (rm_option opt) 
{	
	if(opt != BestFit && opt != WorstFit && opt != FirstFit) {
		printf("Invalid rm_option\n");
		return;
	}	
	
	curr_opt = opt;
}


void 
rmprint () 
{
	rm_header_ptr itr ;
	int i ;

	printf("==================== rm_free_list ====================\n") ;
	for (itr = rm_free_list.next, i = 0 ; itr != 0x0 ; itr = itr->next, i++) {
		printf("%3d:%p:%8d:", i, ((void *) itr) + sizeof(rm_header), (int) itr->size) ;

		int j ;
		char * s = ((char *) itr) + sizeof(rm_header) ;
		for (j = 0 ; j < (itr->size >= 8 ? 8 : itr->size) ; j++) 
			printf("%02x ", s[j]) ;
		printf("\n") ;
	}
	printf("\n") ;

	printf("==================== rm_used_list ====================\n") ;
	for (itr = rm_used_list.next, i = 0 ; itr != 0x0 ; itr = itr->next, i++) {
		printf("%3d:%p:%8d:", i, ((void *) itr) + sizeof(rm_header), (int) itr->size) ;

		int j ;
		char * s = ((char *) itr) + sizeof(rm_header) ;
		for (j = 0 ; j < (itr->size >= 8 ? 8 : itr->size) ; j++) 
			printf("%02x ", s[j]) ;
		printf("\n") ;
	}
	printf("\n") ;

}

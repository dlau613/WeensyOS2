#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include "SortedList.h"


// #define _POSIX_C_SOURCE >= 199309L

int opt_yield;
int insert_yield, delete_yield, search_yield;
static pthread_mutex_t lock;
volatile static int lock_m;
static int sync_m; 
static int sync_s;
static int iterations;
static int threads;

static SortedList_t *list;

struct list_struct {
	SortedList_t * list;
	pthread_mutex_t lock;
	volatile int lock_m;
};

struct args_struct {
	int offset;
	SortedListElement_t ** pointer;
};

static char * rand_key(char * str, size_t size) {
	const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	if (size) {
		--size;
		size_t n;
		for (n = 0; n < size; n++) {
			int key = rand() % (int) (sizeof(charset) -1);
			str[n] = charset[key];
		}
		str[size] = '\0';
	}
	return str;
}

char * rand_key_wrapper(size_t size) {
	char *s = malloc(size+1);
	if (s) {
		rand_key(s,size);
	}
	return s;
}

void * wrapper(void * arg) {
	struct args_struct * args;
	args = arg;
	int offset = args->offset;
	SortedListElement_t ** pointer = args->pointer;
	int i;
	for(i = offset; i < offset+iterations;++i) {
		// printf("%s\n",pointer[i]->key);
		SortedList_insert(list, pointer[i]);
	}
	int length = SortedList_length(list);
	for (i = offset; i < offset+iterations;++i) {
		if (sync_m)
			pthread_mutex_lock(&lock);
		if (sync_s)
			while(__sync_lock_test_and_set(&lock_m,1));

		SortedListElement_t * temp = SortedList_lookup(list, pointer[i]->key);
		if (temp != NULL)
			SortedList_delete(temp);
		
		if (sync_m)
			pthread_mutex_unlock(&lock);
		if (sync_s)	
			__sync_lock_release(&lock_m);
	}	
	return (void*)arg;
}

int SortedList_length(SortedList_t *list) {
	int length = 0;
	SortedListElement_t *n = list->next;
	while (n != list) {
		// if ()
		length++;
		// printf("%s, ",n->key);
		n = n->next;
	}
	// printf("\n");
	return length;
}



SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
	SortedListElement_t *n = list->next;
	while (n != list) {
		if (strcmp(n->key, key) == 0) {
			return n;
		}
		n = n->next;
	}
	return NULL;
}
void SortedList_insert(SortedList_t * list, SortedListElement_t * element) {
	// pthread_mutex_lock(&lock);
	if (sync_m)
		pthread_mutex_lock(&lock);
	if (sync_s)
		while(__sync_lock_test_and_set(&lock_m,1));
	SortedListElement_t *p = list;
	SortedListElement_t *n = list->next;
	while (n != list) {
		if (strcmp(element->key, n->key) < 0) //keep looping till you find an element greater than it
			break;
		n = n->next;
	}
	if (opt_yield && insert_yield) 
		pthread_yield();
	p=n->prev;
	element->prev=p;
	element->next=n;
	p->next=element;
	n->prev=element;

	if (sync_m)
		pthread_mutex_unlock(&lock);
	if (sync_s)	
		__sync_lock_release(&lock_m);
}

int SortedList_delete( SortedListElement_t *element) {
	// if (sync_m)
	// 	pthread_mutex_lock(&lock);
	// if (sync_s)
	// 	while(__sync_lock_test_and_set(&lock_m,1));
	SortedListElement_t *n = element->next;
	SortedListElement_t *p = element->prev;
	if (n->prev != element){
		// if (sync_m)
		// 	pthread_mutex_unlock(&lock);
		// if (sync_s)	
		// 	__sync_lock_release(&lock_m);
		return 1;
	}
	if (p->next != element){
		// if (sync_m)
		// 	pthread_mutex_unlock(&lock);
		// if (sync_s)	
		// 	__sync_lock_release(&lock_m);
		return 1;
	}
	//thread yield
	// if (sync_m)
		// pthread_mutex_lock(&lock);
	// if (sync_s)
	// 	while(__sync_lock_test_and_set(&lock_m,1));
	if (opt_yield && delete_yield) 
		pthread_yield();
	n->prev = p;
	p->next = n;
	element->next = NULL;
	element->prev=NULL;
	// if (sync_m)
	// 	pthread_mutex_unlock(&lock);
	// if (sync_s)	
	// 	__sync_lock_release(&lock_m);
	// free(element);
	return 0;
}

int main(int argc, char *argv[]) 
{
	sync_m = 0;
	sync_s = 0;
	insert_yield = 0;
	delete_yield = 0;
	search_yield = 0;
	iterations = 1;
	threads = 1;
	srand(time(0));
	// SortedList_t *list;
	list = malloc(sizeof(SortedList_t));
	list->key = NULL;
	list->prev = list;
	list->next = list;

	// SortedList_t * list = malloc(sizeof(SortedList_t));
	// list->key = NULL;
	// list->prev = list; 
	// list->next = list;
	// SortedListElement_t *e = malloc(sizeof(SortedListElement_t));
	// e->key = "daniel";
	// SortedList_insert(list,e);


	// char * key = rand_key_wrapper(6);
	// SortedListElement_t *e2 = malloc(sizeof(SortedListElement_t));
	// e2->key = key;
	// SortedList_insert(list,e2);
	// printf("length: %d\n",SortedList_length(list));	 
	
	// free(e);
	// free(e2);
	// free(list);

	while(1)
	{
		static struct option long_options[] =
		{
			{"threads",required_argument,0,'t'},
			{"iterations",required_argument,0,'i'},
			{"yield", required_argument,0,'y'},
			{"sync", required_argument,0,'s'},
			{0, 0, 0, 0}
		};
		int option_index;
		option_index = 0;
		int c;
		c= getopt_long(argc,argv, "", long_options, &option_index);
		if (c==-1)
			break;
		switch(c)
		{
			case 't':
				threads = atoi(optarg);
				break;

			case 'i':
				iterations = atoi(optarg);
				break;

			case 'y':
				// if (strcmp(optarg,"1"))//im lazy for now
					opt_yield = 1;
				int j;
				for (j=0; j< strlen(optarg);j++) {
					if (optarg[j] == 'i') {
						insert_yield = 1;
						// printf("i\n");
					}
					if (optarg[j] == 'd'){
						delete_yield = 1;
						// printf("d\n");
					}
					if (optarg[j] == 's'){
						search_yield = 1;
						// printf("s\n");
					}
				}
				break;

			case 's':
				if (strcmp(optarg, "m")==0) {
					sync_m = 1;
				}
				if (strcmp(optarg, "s")==0)
					sync_s = 1;
				break;



		}
	}
	int i;
	SortedListElement_t ** array = malloc(sizeof(SortedListElement_t*)*threads*iterations);
	for (i = 0; i < threads*iterations;++i) {
		array[i] = malloc(sizeof(SortedListElement_t));
		array[i]->key = rand_key_wrapper(15);
	}
	// struct args_struct pass;
	// pass.pointer = array;
	pthread_t tid[threads];
	struct timespec start,end;
	clock_gettime(CLOCK_REALTIME, &start);

		for (i=0;i <threads; i++) {
			struct args_struct *pass = malloc(sizeof(struct args_struct));
			pass->pointer = array;
			pass->offset = i*iterations;
			pthread_create(&tid[i], NULL, wrapper, (void*)pass);
		}
		for (i=0;i<threads;i++) {
			pthread_join(tid[i],NULL);
		}

	clock_gettime(CLOCK_REALTIME,&end);
	double elapsed_time;
	elapsed_time = ((double)end.tv_sec*pow(10,9) + (double)end.tv_nsec) - 
					((double)start.tv_sec*pow(10,9) + (double)start.tv_nsec);

	
	for (i=0;i < threads*iterations;++i) {
		free(array[i]);
	}

	int length = SortedList_length(list);
	char error[10];
	int exit_status;
	exit_status = 0;
	if (length != 0) {
		fprintf(stderr,"ERROR: length not 0\n");
		strcpy(error,"ERROR: ");
		exit_status = 1;
	}
	else 
		strcpy(error,"");
	printf("%d threads x %d iterations x (insert + delete) = %d operations\n",threads,iterations,threads*iterations*2);
	printf("%sfinal length = %d\n",error, length);
	printf("elapsed time: %f ns\n",elapsed_time);
	printf("per operation: %f ns\n",elapsed_time/(threads*iterations*2));
	free(array);
	free(list);
}




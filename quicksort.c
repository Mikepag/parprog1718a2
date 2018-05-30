#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
				//Default Values:
#define C_BUFFER_SIZE 1000	//1000
#define THREADS 4		//4
#define LIMIT 50000		//50000
#define N 1000000		//1000000
#define CUTOFF 10		//10

struct parameters {
	double *start;		//points to the first cell of the part of the array to be sorted.
	int end;		//points to the last cell of the part of the array to be sorted.
	int shutdown;		//equals 1 when the whole array is sorted
};

struct parameters circular_buffer[C_BUFFER_SIZE];	//array with size = C_BUFFER_SIZE. Each cell contains a parameters struct

int cbuf_in = 0;	//is equal to the number of the circular buffer's cell, into which we can put data.
int cbuf_out = 0;	//is equal to the number of the circular buffer's cell, from which we can get data.

// global avail messages count (0 or 1)
int global_availmsg = 0;	// empty

int global_n_counter = 0;	//n_counter is equal to the number of the last sorted array's cell. If n_counter == N, then whole array[N] is sorted!

// condition variable, signals a put operation (getNewTask() waits on this)
pthread_cond_t msg_in = PTHREAD_COND_INITIALIZER;
// condition variable, signals a get operation (putNewTask() waits on this)
pthread_cond_t msg_out = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;




int inssort(double *a,int n) {
	int i,j;
	double t;

	global_n_counter = global_n_counter + n;
	

	for (i=1;i<n;i++) {
		j = i;
		while ((j>0) && (a[j-1]>a[j])) {
			t = a[j-1];  a[j-1] = a[j];  a[j] = t;
			j--;
		}
	}
	
	if(global_n_counter == N) {	//when n_counter reaches N, whole array is sorted!
		printf("global_n_counter = %d\n", global_n_counter);
		return 1;	//inssort returns 1 if whole array[N] is sorted.
	}
	else {
		return 0;	//inssort returns 0 if array[N] is not fully sorted yet.
	}
}


int partition(double *a, int n) {
	int first, middle, last;
	double p,t;
	int i,j;

	first = 0;
	middle = n-1;
	last = n/2;  

	// put median-of-3 in the middle
	if (a[middle]<a[first]) { t = a[middle]; a[middle] = a[first]; a[first] = t; }
	if (a[last]<a[middle]) { t = a[last]; a[last] = a[middle]; a[middle] = t; }
	if (a[middle]<a[first]) { t = a[middle]; a[middle] = a[first]; a[first] = t; }

	// partition (first and last are already in correct half)
	p = a[middle]; // pivot
	for (i=1,j=n-2;;i++,j--) {
		while (a[i]<p) i++;
		while (p<a[j]) j--;
		if (i>=j) break;
		
		t = a[i]; a[i] = a[j]; a[j] = t;      
	}
	return i;
}


int quicksort(double *a,int n) {
	int i;
	int sorted = 0;	//sorted gets value returned from inssort(). Indicates whether array[N] is sorted or not.

	// check if below cutoff limit
	if (n<=CUTOFF) {
		sorted = inssort(a,n);	//inssort returns 1 if whole array is sorted. Else returns 0.
		return sorted;	//quicksort (just like inssort) returns 1 if whole array is sorted. Else returns 0.
	}

	i = partition(a, n);

	// recursively sort halves
	quicksort(a,i);			//this call can NOT return 1 because it's quicksorting the first half of sub-array[n]
	sorted = quicksort(a+i,n-i);	//if this returns 1, array[N] is sorted.

	return sorted;	//quicksort (just like inssort) returns 1 if whole array is sorted. Else returns 0.
}


void putNewTask(double *a, int n, int shutdown) {	//PUTS new task into circular buffer.

	while ((global_availmsg >= C_BUFFER_SIZE)) {	//while circular buffer is full
		printf("waiting msg_out");
		pthread_cond_wait(&msg_out,&mutex);			// wait until a msg is extracted from buffer - NOTE: mutex MUST be locked here.	
	}
	
	circular_buffer[cbuf_in].start = a;
	circular_buffer[cbuf_in].end = n;
	circular_buffer[cbuf_in].shutdown = shutdown;
	
	

	cbuf_in = cbuf_in +1;
	if( cbuf_in == (C_BUFFER_SIZE-1) )	{	//if cbuf_out reached end of buffer...
		cbuf_out = 0;				//cbuf_out to "points" to start again.
	}
	else {						//else, if cbuf_in didn't reach end of buffer yet...
		cbuf_in = cbuf_in+1;			//increment cbuf_in.
	}

	global_availmsg = global_availmsg +1;
	printf("availmsg = %d\n", global_availmsg);

	pthread_cond_signal(&msg_in);	// signal getNewTask() that a task was added to buffer (used when cbuffer is empty)
}


int getNewTask(double **ptr_a, int *ptr_n) {	//GETS new task from circular buffer.
			
	while (global_availmsg<1) {
		printf("waiting msg_in\n");		//while buffer is Empty
		pthread_cond_wait(&msg_in,&mutex);	//wait until a msg is received in buffer - NOTE: mutex MUST be locked here.
	}

	if(circular_buffer[cbuf_out].shutdown == 1) {
		return 0;	//returning because the whole array is sorted.
	}
	else {
		// receive task from circular buffer:
		*ptr_a = circular_buffer[cbuf_out].start;	//single pointer "a" in thread_func() now points to the same location of array[N] that pointer "start" points. 
		*ptr_n = circular_buffer[cbuf_out].end;		//value of variable "n" in thread_func() now is equal to variable's "end" value			

		cbuf_out = cbuf_out +1;
		if( cbuf_out == (C_BUFFER_SIZE-1) )	{	//if cbuf_out reached end of buffer...
			cbuf_out = 0;				//cbuf_out to "points" to start again.
		}
		else {						//else, if cbuf_out didn't reach end of buffer yet...
			cbuf_out = cbuf_out+1;			//increment cbuf_out.
		}

		global_availmsg = global_availmsg -1;
		printf("availmsg = %d\n", global_availmsg);

		pthread_cond_signal(&msg_out);	// signal putNewTask() that a task was removed from buffer (used when cbuffer is full)

		return 1;	//returning 1 means success getting new task from buffer.
	}		
}


void *thread_func(void *args) {
	double *a;
	double **ptr_a; //points to pointer a. Used for passing address of pointer a to getNewTask() function.
	int n;
	int *ptr_n; //points to variable n. Used for passing address of variable n to getNewTask() function.		
	int i;	
	int getTask_success = -1;
	int sorted = 0;	//sorted gets value returned from quicksort(). Indicates whether array[N] is sorted or not.
	
	ptr_a = &a;
	ptr_n = &n;

	printf("Hello from thread function.\n");

	for( ; ; ) {	//loop forever (until getNewTask() returns 0).
	
		// lock mutex____________________________________________________________________________________________
		pthread_mutex_lock(&mutex);
		
		getTask_success = -1;


		//calling function getNewTask() to get next task from buffer
		getTask_success = getNewTask(ptr_a, ptr_n);


		if(getTask_success == -1) {	//did't successfully got new task from buffer.
			printf("Error, something went wrong while getting task from buffer\n");
		}
		if(getTask_success == 0) {	//received shutdown message.
			printf("\n----- Received shutdown message -----\n\n");
			break;	//because the whole array is sorted.
		}
		if(getTask_success == 1) {	//successfully got new task from buffer.
			printf("Successfully got new task from buffer\n");
			if(n < LIMIT) {
				sorted = quicksort(a, n);
				if(sorted == 1) {			//if quisort returns 1, array[N] is sorted.
					for(i=0; i<THREADS; i++) {
						putNewTask(a, -1, 1);	//sending SHUTDOWN message to other threads
						pthread_cond_signal(&msg_in);
					}					
				}
			}
			else {
				i = partition(a, n);
				
				//ch_0.a = a; 		ch_0.n = i;		Task1: a --> n
    				//ch_1.a = a + i;	ch_1.n = n - i;		Task2: (a+i) --> (n-1)

				putNewTask(a, i, 0);
				putNewTask(a+i, n-i, 0);			
			}
		}
		// unlock mutex__________________________________________________________________________________________
		pthread_mutex_unlock(&mutex);
	}
	// unlock mutex
	pthread_mutex_unlock(&mutex);	//unlocking mutex also here in case a shutdown message is received and we break from for(;;).
	// exit and let be joined
	pthread_exit(NULL);
}





int main() {
	printf("\nHello World!\n");

	double *array;
	int i;
	
	//mallocing array
	array = (double *) malloc(N*sizeof(double));
	if (array==NULL) {
		printf("error in malloc\n");
		exit(1);
	}
	printf("array[] created\n");

	// fill array with random numbers
	srand(time(NULL));
	for (i=0;i<N;i++) {
		array[i] = (double)rand()/RAND_MAX;
	}
	printf("array[] filled with random numbers\n\n");

	//for(i=0; i<N; i++) {
	//	printf("array[%d] = %lf\n", i, array[i]);
	//}
	

	
	//________________________________________________________
	//THREADPOOL
	int thrdnum;

	pthread_t threadPool[THREADS];		//pthread_t : Only used to identify a thread.
	//threadPool[] : table of thread IDs (handles) filled on creation, to be used later on join

	for(thrdnum=0; thrdnum<THREADS; thrdnum++) {
		
		if (pthread_create(&threadPool[thrdnum], NULL, thread_func, NULL)!=0) {
			printf("Error in thread creation!\n");
		exit(1);
		}
		printf("Thread %d created!\n", thrdnum);
	}
	
	putNewTask(array, N, 0);	//Creates the first task which includes the whole array[N]
	

	//join threads
	for(thrdnum=0; thrdnum<THREADS; thrdnum++) {
		//pthread_mutex_unlock(&mutex);
		pthread_join(threadPool[thrdnum], NULL);
		printf("Thread %d joined!\n", thrdnum);
	}
	//________________________________________________________

	


	// check sorting
	for (i=0;i<(N-1);i++) {
		if (array[i]>array[i+1]) {
			printf("Sort failed!\n");
			break;
		}
	}
	
	printf("\nARRAY SORTED SUCCESSFULLY!\n\n");


	free(array);

	// destroy mutex - should be unlocked
	pthread_mutex_destroy(&mutex);


	return 0;
}

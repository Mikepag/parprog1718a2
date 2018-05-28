#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define C_BUFFER_SIZE 1000
#define THREADS 4
#define LIMIT 50000
#define N 1000000

struct parameters {
	double *start;		//points to the first cell of the part of the array to be sorted.
	int end;			//points to the last cell of the part of the array to be sorted.
	int shutdown;		//equals 1 when the whole array is sorted
};

struct parameters circular_buffer[C_BUFFER_SIZE];	//array with size = C_BUFFER_SIZE. Each cell contains a parameters struct

int cbuf_in = 0;	//is equal to the number of the circular buffer's cell, into which we can put data.
int cbuf_out = 0;	//is equal to the number of the circular buffer's cell, from which we can get data.

// global avail messages count (0 or 1)
int global_availmsg = 0;	// empty
// condition variable, signals a put operation (getNewTask() waits on this)
pthread_cond_t msg_in = PTHREAD_COND_INITIALIZER;
// condition variable, signals a get operation (putNewTask() waits on this)
pthread_cond_t msg_out = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;




void putNewTask(double *a, int n, int shutdown) {	//PUTS new task into circular buffer.

	while ((global_availmsg >= C_BUFFER_SIZE)) {	//while circular buffer is full
		pthread_cond_wait(&msg_out,&mutex);			// wait until a msg is extracted from buffer - NOTE: mutex MUST be locked here.
	}
	
	circular_buffer[cbuf_in].start = a;
	circular_buffer[cbuf_in].end = n;
	circular_buffer[cbuf_in].shutdown = shutdown;
	
	

	cbuf_in = cbuf_in +1;
	if( cbuf_in == (C_BUFFER_SIZE-1) )	{	//if cbuf_out reached end of buffer...
		cbuf_out = 0;						//cbuf_out to "points" to start again.
	}
	else {									//else, if cbuf_in didn't reach end of buffer yet...
		cbuf_in = cbuf_in+1;				//increment cbuf_in.
	}

	global_availmsg = global_availmsg +1;

	pthread_cond_signal(&msg_in);	// signal getNewTask() that a task was added to buffer (used when cbuffer is empty)
}


int getNewTask(double **ptr_a, int *ptr_n) {	//GETS new task from circular buffer.
			
	while (global_availmsg<1) {				//while buffer is Empty
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
			cbuf_out = 0;						//cbuf_out to "points" to start again.
		}
		else {									//else, if cbuf_out didn't reach end of buffer yet...
			cbuf_out = cbuf_out+1;				//increment cbuf_out.
		}

		global_availmsg = global_availmsg -1;

		pthread_cond_signal(&msg_out);	// signal putNewTask() that a task was removed from buffer (used when cbuffer is full)

		return 1;	//returning 1 means success getting new task from buffer.
	}		
}


void *thread_func(void *args) {
	double *a;
	double **ptr_a; //points to pointer a. Used for passing address of pointer a to getNewTask() function.
	int n;
	int *ptr_n; //points to variable n. Used for passing address of variable n to getNewTask() function.		
	//int i;	
	int getTask_success = -1;
	
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
			printf("\nReceived shutdown message\n\n");
			break;	//because the whole array is sorted.
		}
		if(getTask_success == 1) {	//successfully got new task from buffer.
		
		////////////////////////////////////////
			if(n < LIMIT) {
				//quicksort(a, n);		<<<<<< NOT DEFINED YET
			}
			else {
				//i = partition(a, n);	<<<<<< NOT DEFINED YET
				//putNewTask(a, i, 0);
				//putNewTask(i, n, 0);
		////////////////////////////////////////			
			}
		}
		// unlock mutex__________________________________________________________________________________________
		pthread_mutex_unlock(&mutex);
	}
	// exit and let be joined
	pthread_exit(NULL);
}





int main() {
	printf("Hello World!\n");

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
	
	//putNewTask(0, N, 1);

	for(thrdnum=0; thrdnum<THREADS; thrdnum++) {
		pthread_join(threadPool[thrdnum], NULL);
		printf("Thread %d joined!\n", thrdnum);
	}

	//________________________________________________________


	return 0;
}

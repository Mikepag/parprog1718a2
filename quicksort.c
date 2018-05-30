#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
				// Default Values:
#define C_BUFFER_SIZE 1000	// 1000
#define THREADS 4		// 4
#define LIMIT 50000		// 50000
#define N 1000000		// 1000000
#define CUTOFF 10		// 10

// Stuct containing task's parameters. (Each circular buffer's cell consists of a task_parameters struct). 
struct task_parameters {
	double *start;		// Points to the first cell of of the array that's about to be sorted.
	int end;		// Is equal to the size of the the array that's about to be sorted.
	int shutdown;		// Equals 1 when the whole array is sorted.
};

// Circular Buffer
struct task_parameters circular_buffer[C_BUFFER_SIZE];	// Array with size == C_BUFFER_SIZE. Each cell of circular_buffer represents a TASK and contains a task_parameters struct.
							// (Actualy what it is, is an array of structs).

int cbuf_in = 0;	// Is equal to the number of the circular buffer's cell, into which we can put data.
int cbuf_out = 0;	// Is equal to the number of the circular buffer's cell, from which we can get data.

// Available messages counter is equal to the current number of messages (tasks) inside circular buffer.
int global_availmsg = 0;	// Initialized with zero because circular buffer is still empty.

// n_counter is equal to the current array's sorted part size. If n_counter == N, then the whole array[N] is sorted!
//(Each time a new small array is sorted with insort, n_counter gets its size and adds it to the current sorted array's size ).
int global_n_counter = 0;	// Initialized with zero because no cells of the array are sorted yet.

// Condition variable, signals a put operation. (getNewTask() waits on this when circular buffer is Empty).
pthread_cond_t msg_in = PTHREAD_COND_INITIALIZER;
// Condition variable, signals a get operation. (putNewTask() waits on this when circular buffer is Full).
pthread_cond_t msg_out = PTHREAD_COND_INITIALIZER;

// Mutex. Used to block some critical parts of code in order to allow only one thread at a time to access them.
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;




// inssort() originally was void. Now it's int because it returns value 1 when the whole array is sorted (using global_n_counter to check this).
int inssort(double *a,int n) {
	int i,j;
	double t;

	for (i=1;i<n;i++) {
		j = i;
		while ((j>0) && (a[j-1]>a[j])) {
			t = a[j-1];  a[j-1] = a[j];  a[j] = t;
			j--;
		}
	}
	
	// Adds new array's size (argument n) to the old sorted array's size, so now, n_counter is equal to the current sorted array's size.
	global_n_counter = global_n_counter + n;
	

	if(global_n_counter == N) {	//when n_counter reaches N, the array[N] is sorted!
		printf("global_n_counter = %d\n", global_n_counter);
		return 1;	//inssort returns 1 if the whole array[N] is sorted.
	}
	else {
		return 0;	//inssort returns 0 if array[N] is NOT fully sorted yet.
	}
}


int partition(double *a, int n) {	// Didn't make any changes to partition(). Used as it is in: https://gist.github.com/mixstef/322145437c092783f70f243e47769ac6
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


// quicksort() originally was void. Now it's int because it returns value 1 when the whole array is sorted (using value returned from inssort() to check this).
int quicksort(double *a,int n) {
	int i;
	int sorted = 0;			// Variable sorted indicates whether array[N] is sorted or not. ( Gets value returned from inssort() ).

	// check if below cutoff limit
	if (n<=CUTOFF) {
		sorted = inssort(a,n);	// inssort() returns 1 if whole array is sorted. Else returns 0.
		return sorted;		// quicksort(), just like inssort(), returns 1 if whole array is sorted. Else returns 0.
	}

	i = partition(a, n);

	// recursively sort halves
	quicksort(a,i);			// This call of quicksort() can NOT return 1 because it's "quicksorting" the first half of array[n], so the second half is surely not sorted yet. 
	sorted = quicksort(a+i,n-i);	// Variable sorted gets value returned from quicksort() call for the second half of array[n]. If this returns 1, array[N] is sorted.

	return sorted;			// quicksort(), just like inssort(), returns 1 if the whole array[N] is sorted. Else returns 0.
}


void putNewTask(double *a, int n, int shutdown) {	// PUTS a new task into circular buffer.

	while ((global_availmsg >= C_BUFFER_SIZE)) {	// While circular buffer is full...
		printf("waiting msg_out");
		pthread_cond_wait(&msg_out,&mutex);	// Wait until a msg is extracted from buffer - NOTE: mutex MUST be locked here.	
	}
	
	// Put new task in circular buffer:
	circular_buffer[cbuf_in].start = a;		// Passing argument values to circular buffer.
	circular_buffer[cbuf_in].end = n;		// Passing argument values to circular buffer.
	circular_buffer[cbuf_in].shutdown = shutdown;	// Passing argument values to circular buffer.
	
	

	cbuf_in = cbuf_in +1;			// Increment cbuf_in.
	if( cbuf_in == (C_BUFFER_SIZE-1) ) {	// If cbuf_in reached end of buffer...
		cbuf_in = 0;			// cbuf_in "points" to start again.
	}


	global_availmsg = global_availmsg +1;		// Incrementing global_availmsg because a new message (task) was added to circular buffer.
	printf("availmsg = %d\n", global_availmsg);

	pthread_cond_signal(&msg_in);	// Signal getNewTask() that a task was added to buffer (used when cbuffer is empty)
}


int getNewTask(double **ptr_a, int *ptr_n) {		// GETS a new task from circular buffer.
			
	while (global_availmsg<1) {			// While buffer is Empty...
		printf("waiting msg_in\n");
		pthread_cond_wait(&msg_in,&mutex);	// Wait until a msg is added in buffer - NOTE: mutex MUST be locked here.
	}

	if(circular_buffer[cbuf_out].shutdown == 1) {	// If task's shutdown variable == 1...
		cbuf_out = cbuf_out +1;			// Increment cbuf_out.
		if( cbuf_out == (C_BUFFER_SIZE-1) ) {	// If cbuf_out reached end of buffer...
			cbuf_out = 0;			// cbuf_out "points" to start again.
		}
		global_availmsg = global_availmsg -1;	// Reduce global_availmsg by 1 because a message (task) was removed from circular buffer.		
		
		return 0;				// Returning 0 because the whole array is sorted.
	}
	else {
		// Receive task from circular buffer:
		*ptr_a = circular_buffer[cbuf_out].start;	// Single pointer "a" in thread_func() now points to the same location that pointer "start" points. 
		*ptr_n = circular_buffer[cbuf_out].end;		// Value of variable "n" in thread_func() now is equal to variable's "end" value.			

		cbuf_out = cbuf_out +1;				// Increment cbuf_out.
		if( cbuf_out == (C_BUFFER_SIZE-1) ) {		// If cbuf_out reached end of buffer...
			cbuf_out = 0;				// cbuf_out "points" to start again.
		}

		global_availmsg = global_availmsg -1;		// Reduce global_availmsg by 1 because a message (task) was removed from circular buffer.
		printf("availmsg = %d\n", global_availmsg);

		pthread_cond_signal(&msg_out);	// Signal putNewTask() that a task was removed from buffer (used when cbuffer is full)

		return 1;			// Returning 1 means success getting new task from buffer.
	}		
}


void *thread_func(void *args) {
	double *a;
	double **ptr_a; 		// Points to pointer a. Used for passing address of pointer a to getNewTask() function.
	int n;
	int *ptr_n; 			// Points to variable n. Used for passing address of variable n to getNewTask() function.		
	int i;	
	int getTask_success = -1;	// getTask_success get returned value from getNewTask().
	int sorted = 0;			// Variable sorted gets value returned from quicksort(). Indicates whether array[N] is sorted or not.
	
	ptr_a = &a;	// Points to pointer a.
	ptr_n = &n;	// Points to variable n.

	printf("Hello from thread function.\n");

	for( ; ; ) {	// Loop forever (until getNewTask() returns 0).
	
		// Lock mutex____________________________________________________________________________________________
		pthread_mutex_lock(&mutex);
		
		getTask_success = -1;	// Initialization.


		// Calling function getNewTask() to get next task from buffer
		getTask_success = getNewTask(ptr_a, ptr_n);


		if(getTask_success == -1) {	// Did't successfully got new task from buffer.
			printf("Error, something went wrong while getting task from buffer\n");
		}
		if(getTask_success == 0) {	// Received shutdown message.
			printf("\n----- Received shutdown message -----\n\n");
			break;	// Because the whole array is sorted.
		}
		if(getTask_success == 1) {	// Successfully got new task from buffer.
			printf("Successfully got new task from buffer\n");
			if(n < LIMIT) {
				sorted = quicksort(a, n);
				if(sorted == 1) {			// If quisort returns 1, array[N] is sorted.
					for(i=0; i<THREADS; i++) {
						putNewTask(a, -1, 1);	// Send SHUTDOWN message to other threads.
					}					
				}
			}
			else {
				i = partition(a, n);
				
				//ch_0.a = a; 		ch_0.n = i;		Task1: a --> n
    				//ch_1.a = a + i;	ch_1.n = n - i;		Task2: (a+i) --> (n-i)

				putNewTask(a, i, 0);
				putNewTask(a+i, n-i, 0);			
			}
		}
		// Unlock mutex__________________________________________________________________________________________
		pthread_mutex_unlock(&mutex);
	}
	// unlock mutex
	pthread_mutex_unlock(&mutex);	// Unlocking mutex also here in case a shutdown message is received and we break from for(;;) in line 212.
	// exit and let be joined
	pthread_exit(NULL);
}





int main() {
	printf("\nHello World!\n");

	double *array;
	int i;
	
	//mallocing array[N]
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
	int thrdnum;	//just a counter.

	pthread_t threadPool[THREADS];		//pthread_t : Only used to identify a thread.
	//threadPool[] : table of thread IDs (handles) filled on creation, to be used later on join.

	for(thrdnum=0; thrdnum<THREADS; thrdnum++) {
		
		// Create threads:
		// Thread IDs are stored in threadPool[thrdnum] (1st argument).
		// When a thread is created, starts running in thread_func() (3rd argument).
		if (pthread_create(&threadPool[thrdnum], NULL, thread_func, NULL)!=0) {
			printf("Error in thread creation!\n");
			exit(1);
		}
		printf("Thread %d created!\n", thrdnum);
	}
	

	putNewTask(array, N, 0);	// Creates the first task which includes the whole array[N] and shutdown message == 0.
	

	// Join threads:
	for(thrdnum=0; thrdnum<THREADS; thrdnum++) {
		pthread_join(threadPool[thrdnum], NULL);
		printf("Thread %d joined!\n", thrdnum);
	}
	//________________________________________________________

	


	// Check sorting
	for (i=0;i<(N-1);i++) {
		if (array[i]>array[i+1]) {
			printf("Sort failed!\n");
			break;	// Break if array[N] was not sorted.
		}
	}
	
	printf("\nARRAY SORTED SUCCESSFULLY!\n\n");


	free(array);

	// destroy mutex - should be unlocked
	pthread_mutex_destroy(&mutex);


	return 0;
}

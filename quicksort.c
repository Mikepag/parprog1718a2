#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define C_BUFFER_SIZE 1000
#define THREADS 4

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

// condition variable, signals a put operation (receiver waits on this)
pthread_cond_t msg_in = PTHREAD_COND_INITIALIZER;
// condition variable, signals a get operation (sender waits on this)
pthread_cond_t msg_out = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;



void *thread_func(void *args) {
	double *a;
  int n;
  int i;	

	printf("Hello from thread function.\n");

	for( ; ; ) {
		
		// lock mutex
  	pthread_mutex_lock(&mutex);

		while (global_availmsg<1) {	// NOTE: we use while instead of if! more than one thread may wake up
  		pthread_cond_wait(&msg_in,&mutex);	// wait until a msg is received - NOTE: mutex MUST be locked here.
		}

		if(circular_buffer[cbuf_out].shutdown == 1) {
  		break;	//terminating for(;;) loop using break statement because the whole array is sorted.
		}
		else {
			// receive task from circular buffer:
			a = circular_buffer[cbuf_out].start;
			n = circular_buffer[cbuf_out].end;
		
			global_availmsg = global_availmsg -1;
 
			// signal the sender that something was removed from buffer
		  pthread_cond_signal(&msg_out);
	
			// unlock mutex
		  pthread_mutex_unlock(&mutex);
		}
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
	

	for(thrdnum=0; thrdnum<THREADS; thrdnum++) {
		pthread_join(threadPool[thrdnum], NULL);
		printf("Thread %d joined!\n", thrdnum);
	}

	//________________________________________________________


	return 0;
}

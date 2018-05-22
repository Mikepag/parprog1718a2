#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define C_BUFFER_SIZE 1000
#define THREADS 4

struct parameters
{
	double *start;		//points to the first cell of the part of the array to be sorted.
	int end;			    //points to the last cell of the part of the array to be sorted.
	int shutdown;		  //equals 1 when the whole array is sorted.
};

struct parameters circular_buffer[C_BUFFER_SIZE];	//array with size = C_BUFFER_SIZE. Each cell contains a parameters struct

int q_insert = 0;	//is equal to the number of the circular buffer's cell, into which we can put data.
int q_output = 0;	//is equal to the number of the circular buffer's cell, from which we can get data.

// condition variable, signals a put operation (receiver waits on this)
pthread_cond_t msg_in = PTHREAD_COND_INITIALIZER;
// condition variable, signals a get operation (sender waits on this)
pthread_cond_t msg_out = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;



void *thread_func(void *args)
{
	printf("Hello from thread function.\n");
}



int main()
{
	printf("Hello World!\n");

//________________________________________________________
//THREADPOOL
	int thrdnum;

	pthread_t threadPool[THREADS];		//pthread_t : Ôype used to identify a thread.
	//threadPool[] : table of thread IDs (handles) filled on creation, to be used later on join

	for(thrdnum=0; thrdnum<THREADS; thrdnum++)
	{
		
		if (pthread_create(&threadPool[thrdnum], NULL, thread_func, NULL)!=0)
		{
     		printf("Error in thread creation!\n");
      		exit(1);
		}
		printf("Thread %d created!\n", thrdnum);
	}
	

	for(thrdnum=0; thrdnum<THREADS; thrdnum++)
	{
		pthread_join(threadPool[thrdnum], NULL);
		printf("Thread %d joined!\n", thrdnum);
	}

//________________________________________________________


	return 0;
}

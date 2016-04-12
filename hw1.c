/*
* Author: Matthew Taylor
* Homework: Concurrency 1
* Date: 4/8/2016
*
*REFERENCE: Intel https://software.intel.com/en-us/articles/intel-digital-random-number-generator-drng-software-implementation-guide
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "mt19937ar.c"

#define MAX 32

#define _rdrand_generate(x) ({ unsigned char err; asm volatile("rdrand %0; setc %1":"=r"(*x), "=qm"(err)); err;})

struct buffer {
	int value;
	int waitTime;
};

struct buffer_list {
	struct buffer buffs[32];
	int index;
};

typedef struct uint32_struct{
	unsigned int value;
} uint32_t;


pthread_mutex_t mutex;
pthread_cond_t pcond, ccond;

#define DRNG_NO_SUPPORT 0x0 /* For clarity */
#define DRNG_HAS_RDRAND 0x1
#define DRNG_HAS_RDSEED 0x2

int rdrand32_step(uint32_t *random){
	unsigned char ok;

	asm volatile ("rdrand %0; setc %1"
		: "=r" (*random), "=qm" (ok));

	return (int) ok;
}


unsigned int eax,ebx,ecx,edx;

struct buffer_list buffList = {
	.buffs = { 
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0},
		{0 , 0}
	 },
	 .index = 0
};

pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sigToConsumer = PTHREAD_COND_INITIALIZER;
pthread_cond_t sigToProducer = PTHREAD_COND_INITIALIZER;

/**********************
* FUNCTION: CONSUMERS * 
**********************/
void *consumer(void *dummy) {
	int i;
	for(i = 0; i < MAX; i++) {
		int waitTime = 0;
		pthread_mutex_lock(&mu);
		// Waiting for Producer to put a new buffer in
		while (buffList.buffs[i].value == 0) 
	    {
	    	pthread_cond_wait(&sigToConsumer, &mu);
	    }
		//pthread_cond_wait(&sigToConsumer, &mu);
		waitTime = buffList.buffs[i].waitTime;
		pthread_cond_signal(&sigToProducer);
		pthread_mutex_unlock(&mu);
		// Sleep for 2 to 9 seconds
		printf("Consumer: Pausing for %d seconds.\n", waitTime);
		sleep(waitTime);
		// Print the current index + the value of the current buffer
		printf("Consumer -> (Value: %d)\n", buffList.buffs[i].value);
		/* Unlock the mutex. */
		buffList.buffs[i].value = 0;
	}
	pthread_exit(0);
}

/**********************
* FUNCTION: PRODUCERS * 
**********************/
void *producer(void *dummy) {
	int i;
	for(i = 0; i < MAX; i++) {
		int waitTimeCons, waitTimeSelf, value;
		char vendor[13];

		eax = 0x01;
		__asm__ __volatile__(
		                     "cpuid;"
		                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
		                     : "a"(eax)
		                     );
		
		if(ecx & 0x40000000)
		{
			//use rdrand
			uint32_t randVal;
			randVal.value;

			// Retrieving values
			waitTimeCons = (int)rdrand32_step(&randVal);
			if(waitTimeCons) 
			{
				waitTimeCons = (int)randVal.value;
			}
			value = (int)rdrand32_step(&randVal);
			if(value) 
			{
				value = (int)randVal.value;
			}
			waitTimeSelf = (int)rdrand32_step(&randVal);
			if(waitTimeSelf) 
			{
				waitTimeSelf = (int)randVal.value;
			}

	    }
	    else
	    {
	        init_genrand(time(NULL));
			waitTimeCons = (int)genrand_int32();
			value = (int)genrand_int32();
			waitTimeSelf = (int)genrand_int32();
	    }
	    // key values
	    waitTimeCons = (abs(waitTimeCons) % 8) + 2; // 2 to 9
	    value = (abs(value) % 10) + 1; // 1 to 10
	    waitTimeSelf = (abs(waitTimeSelf) % 5) + 3; // 3 to 7

	    pthread_mutex_lock(&mu);

	    // wait for confirm
	    while (buffList.buffs[i].value != 0) 
	    {
      		pthread_cond_wait(&sigToProducer, &mu);
      	}

      	// Storing the value waittime in the current buffer in buffList
		buffList.buffs[i].value = value;
		buffList.buffs[i].waitTime = waitTimeCons;

      	// Tells Consumer that there is something in the buff list now
	    pthread_cond_signal(&sigToConsumer);

	    // mu unlocked
	    pthread_mutex_unlock(&mu);
	    printf("Producer: Pausing for %d seconds.\n", waitTimeSelf);
	    // Producer snoozin'
		sleep(waitTimeSelf);
	}
	pthread_exit(0);
}

void main() {
	pthread_t produce, consume;

	// init mutex
	pthread_mutex_init(&mu, NULL);	
	
	// Condition Variables
	pthread_cond_init(&sigToConsumer, NULL);
	pthread_cond_init(&sigToProducer, NULL);

	// Thread Creation
	pthread_create(&consume, NULL, consumer, NULL);
	pthread_create(&produce, NULL, producer, NULL);

	// Once threads finish, kill 'em
	pthread_join(consume, NULL);
	pthread_join(produce, NULL);

	// Cleanup
	pthread_mutex_destroy(&mu);	
	pthread_cond_destroy(&sigToConsumer);
	pthread_cond_destroy(&sigToProducer);
}

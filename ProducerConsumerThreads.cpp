#include <iostream>
#include <string>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <stdlib.h>
#include "unistd.h" 
#include <pthread.h>   

// g++ -std=c++11 -o project3 project3.cpp -lpthread
// ./project3 TP TC

#define BUFFER_SIZE 100
#define PRODUCERS 10
#define CONSUMERS 10

//used for the "infinite" loop
#define MAX 10000

#define EOD -1

using namespace std;
typedef struct {
    int value[BUFFER_SIZE];
    int next_in, next_out;
} BBQ;

int checkCounter = 0;
int maxCounter = 0;
int minCounter = 0;
int TP = 0;
int TC = 0;
BBQ buffer;

pthread_t consumer_tid[CONSUMERS];
pthread_t producer_tid[PRODUCERS];
pthread_mutex_t MonitorLock2;
pthread_mutex_t ScreenLock2;
pthread_cond_t notFull2;
pthread_cond_t notEmpty2;

int insert_item(int item, long int id)
{
    pthread_mutex_lock(&MonitorLock2);

    while(checkCounter == BUFFER_SIZE)
    {
        pthread_cond_wait(&notFull2, &MonitorLock2); 
        minCounter++; 
    }
    pthread_cond_signal(&notEmpty2);
    checkCounter++;
    buffer.value[buffer.next_in] = item;
    buffer.next_in = (buffer.next_in + 1) % BUFFER_SIZE;
    pthread_mutex_unlock(&MonitorLock2);
    
    return item;
}

int remove_item()
{
    int item;
    pthread_mutex_lock(&MonitorLock2);
    while(checkCounter <= 0)
    {
    	pthread_cond_wait(&notEmpty2, &MonitorLock2);
    	maxCounter++;
    }
    pthread_cond_signal(&notFull2);
    checkCounter--;
    item = buffer.value[buffer.next_out];
    buffer.value[buffer.next_out] = -1;
    buffer.next_out = (buffer.next_out + 1) % BUFFER_SIZE;
    pthread_mutex_unlock(&MonitorLock2);

    return item;
}

void *producer(void *param)
{
    int item, i;
    long int id = (long int)param;
    pthread_mutex_lock(&ScreenLock2);
      printf("producer started\n");
    pthread_mutex_unlock(&ScreenLock2);

    for (int i = 0; i < MAX; i++) 
    {
    	int tempTP = TP;
    	if(checkCounter >= (.25*BUFFER_SIZE))
    		tempTP = (TP*.75);
    	else if(checkCounter >= (.1*BUFFER_SIZE))
    		tempTP = (TP*.5);
    	else if (checkCounter > 0 && checkCounter <= (.75*BUFFER_SIZE))
    		tempTP = (TP*1.5);
    	else if (checkCounter == 0)
    		tempTP = (TP*2);	
	usleep(tempTP);
		
	item = rand() % 100;
	insert_item(item, id);

	   pthread_mutex_lock(&ScreenLock2);
	     printf("producer %ld: inserted %d\n", id, item);
	   pthread_mutex_unlock(&ScreenLock2);
    }

    insert_item(EOD, id);
    pthread_mutex_unlock(&ScreenLock2);
    pthread_exit(0);
}

void *consumer(void *param)
{
    int item;
    long int id = (long int)param;
    pthread_mutex_lock(&ScreenLock2);
      printf("consumer started\n");
    pthread_mutex_unlock(&ScreenLock2);

    do{
	usleep(TC);
	pthread_mutex_lock(&ScreenLock2);
	  printf("Consumer %ld is waiting...\n", id);
	pthread_mutex_unlock(&ScreenLock2);
	item = remove_item();
	if (item != EOD)
	{
	    pthread_mutex_lock(&ScreenLock2);
	      printf("consumer %ld: removed %d\n", id, item);
	    pthread_mutex_unlock(&ScreenLock2);
	}
     } while (item != EOD);
	pthread_mutex_lock(&ScreenLock2);
        printf("Consumer %d receives EOD and exits\n", item);
        pthread_mutex_unlock(&ScreenLock2);

    pthread_exit(0);
}

int main(int argc, char *argv[])
{
    long int i;
    char* p;
    long int tpNum;
    long int tcNum;
    
    tpNum = stol(argv[1], NULL,  10);
    tcNum = stol(argv[2], NULL, 10);
    
    TP = tpNum;
    TC = tcNum;

    srand(time(NULL));
   

     /*Create the consumer threads */
    for (i = 0; i < CONSUMERS; i++)
	if (pthread_create(&consumer_tid[i], NULL, consumer, (void *)i) != 0) {
	    perror("pthread_create");
	    abort();
	}
     /*Create the producer threads */
    for (i = 0; i < PRODUCERS; i++)
	if (pthread_create(&producer_tid[i], NULL, producer, (void *)i) != 0) {
	    perror("pthread_create");
	    abort();
	}

    /* Wait for them to complete */
    for (i = 0; i < CONSUMERS; i++)
	if (pthread_join(consumer_tid[i], NULL) != 0) {
	    perror("pthread_join");
	    abort();
	}
    for (i = 0; i < PRODUCERS; i++)
	if (pthread_join(producer_tid[i], NULL) != 0) {
	    perror("pthread_join");
	    abort();
	}
	
	printf("The max buffer has reached %d \n", maxCounter);
	printf("The min buffer has reached %d \n", minCounter);

    return 0;
}

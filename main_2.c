#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>

#define CMD_START_TIMER		    0xffc1
#define CMD_STOP_TIMER		    0xffc2
#define CMD_SET_TIMER_COUNTER_VALUE 0xffc3
#define NUM_OF_SENDER_THREADS	    6
#define NUM_OF_TOKENS_PER_THREAD    100

/* file descriptors for Squeue1, Squeue2 and HRT */
int squeue1_fp,squeue2_fp,hrt_fp;

/* number of tokens read. */
int num_of_tokens_read = 0;

/* structure which is going to be passed and retrieved for read and write */
struct Queue_elem {
	int token_id;
	int time_stamp1;
	int time_stamp2;
	int time_stamp3;
	int time_stamp4;
	char string[81];
};

/* print the structure */
void print_queue_elem(struct Queue_elem* elem)
{
	printf("token id: %d \ntime stamp1: %d \ntime stamp2: %d \ntime stamp3: %d \ntime stamp4: %d \nstring : %s\n", 
	elem->token_id,elem->time_stamp1,elem->time_stamp2, 
	elem->time_stamp3, elem->time_stamp4, elem->string);
}


/* will read the tokens from a queue until it is empty. */
void read_queue(int squeue_fp)
{
	
	struct Queue_elem elem;
	int ret = 0;
	int time;
	
	ret = read(squeue_fp,&elem,sizeof(struct Queue_elem));
	
	while (ret > 0 )
	{
		/* assigning the fourth time stamp */
		read(hrt_fp,&time,sizeof(int));
		elem.time_stamp4 = time;
		
		/* printing the elem which is recently dequeued */
		print_queue_elem(&elem);
		
		/* total number of tokens read until now. */
		num_of_tokens_read++;
		
		/* read a new token */
		ret = read(squeue_fp,(char*)&elem,sizeof(struct Queue_elem));
    	}
	
	/* -1 indicated Queue Empty. If not -1 then there is some error */
	if (ret != -1)
	    printf("read error\n");	
    
}

/* receiver thread calls this function */
void *receive_data(void * id)
{
	int i = (int)id;
	int sleep_time;
	while (num_of_tokens_read < (NUM_OF_SENDER_THREADS * NUM_OF_TOKENS_PER_THREAD))
	{
		/* randomly generate number between 1000 and 10000 */
		sleep_time = generate_random_number(1,10)*1000;
		
		/* sleep for the time indicated in sleep_time */
		usleep(sleep_time);
    
		/* read squeue1 until it is empty */
		read_queue(squeue1_fp);
		
		/* sleep again */
		sleep_time = generate_random_number(1,10)*1000;
		usleep(sleep_time);
		
		/* read squeue2 until it is empty */
		read_queue(squeue2_fp);
	}
}

/* Generate random number between lower_limit and upper_limit */
int generate_random_number(int lower_limit, int upper_limit)
{
	int rand_num = rand();
	return (lower_limit + ((float)rand_num/(float)RAND_MAX) * (upper_limit-lower_limit));
}


/* Sender thread calls this function */
void * send_data(void *id)
{
	int i = (int)id;
	int j,string_length;
	int sent_tokens = 0;
	
	while (sent_tokens < NUM_OF_TOKENS_PER_THREAD)
	{
		char string[81];
		int sleep_time;
		struct Queue_elem elem;
		int time;
		/* generating random string length */
		string_length = generate_random_number(10,80);
		for (j=0;j<string_length;j++)
		{
			/* generating random characters by passing ASCII values */
			string[j] = generate_random_number(97,122);
		}
		/* appending the string with terminator */
		string[string_length] = '\0';
	    
		/* read HRT for the first time stamp */
		read(hrt_fp,&time,sizeof(int));

		/* assigning first time stamp. */
		elem.time_stamp1 = time;
		strcpy(elem.string,string);
		
		/* sleep for random amount of time */
		sleep_time = generate_random_number(1,10)*1000;
		usleep(sleep_time);

		/* select the queue depending on thread id */
		if(i % 2 == 0)
		{
			int res; 
			/* try to write untill successful */
			do{
			res = write(squeue1_fp,(char*)&elem,sizeof(struct Queue_elem));	
			} while(res < 0);
		}
		else
		{
			int res;
			/* try to write untill successful */
			do{
			res = write(squeue2_fp,(char*)&elem,sizeof(struct Queue_elem));	
			} while (res < 0);
		}
		sent_tokens++;
	}
}

int main (int argc, char **argv)
{
	
	int i;
	
	/* structs for the threads. */	
	pthread_t sender_threads[NUM_OF_SENDER_THREADS];
	pthread_t receiver_thread;	   
 
	/* opening Squeue1 and Squeue2 devices. */
	squeue1_fp = open ("/dev/Squeue1",O_RDWR);
	squeue2_fp = open ("/dev/Squeue2",O_RDWR);

	/* opening HRT Device. */
	hrt_fp = open ("/dev/HRT",O_RDWR);

        /* Initialize the HRT to 0 and start it. */
	ioctl(hrt_fp,CMD_SET_TIMER_COUNTER_VALUE,0);
	ioctl(hrt_fp,CMD_START_TIMER);
	
	/* Creat Sender threads */
	for (i=0;i<NUM_OF_SENDER_THREADS;i++)	
	{
		pthread_create(&sender_threads[i],NULL,send_data,(void *)i);	
    	}
	i++;
	/* create receiver thread */
	pthread_create(&receiver_thread,NULL,receive_data,(void*)i);
	
	/* wait for the sender threads to complete */	
	for (i=0;i<NUM_OF_SENDER_THREADS;i++)	
	{
		pthread_join(sender_threads[i],NULL);
	}
	/* wait for the receiver threads to complete */
	pthread_join(receiver_thread,NULL);
	
	/* stop the timer */
	ioctl(hrt_fp,CMD_STOP_TIMER);
		
	/* close the file pointers relating to Queues and HRT */
	close(squeue1_fp);
	close(squeue2_fp);
	close(hrt_fp);
	
}



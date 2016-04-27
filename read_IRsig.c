//compile using gcc -lmraa -o read_IRsig.exe read_IRsig.c gpio.c
#include <stdio.h>
#include <unistd.h>
#include "gpio.h"
#include <sys/time.h>
#include "queue.h"
#include <signal.h>


int message[4];

struct timeval tv_prev_sample;
int preamble_count = 0;
int state = 0;  //0 = looking for preamble  1 = reading the message  2 = looking for postamble  3 = message received

GPIO gpio = GPIO44;

//waits appropriate amount of time before sampling next value from GPIO input
//inputs are the timeval of the previous sampling time and the gpio
//returns bit read from GPIO input
void get_next_bit() {

	//struct timeval tv_now, tv_elapsed;
	int value;
	

		//gettimeofday(&tv_now, NULL);	//get new timestamp
		//timersub(&tv_now, &tv_prev_sample, &tv_elapsed); 	//compare new to prev
		
		//if ((long int)tv_elapsed.tv_usec >= 10000)	//if 10ms elapsed
		
			//printf("time elapsed = %d secs %d microsecs\t", (int) tv_elapsed.tv_sec, (int) tv_elapsed.tv_usec);
		value = !gpio_read(gpio);			//read bit
		fflush(stdout);
		//tv_prev_sample = tv_now;
		printf("just read bit: %d\n", value);//|  tv_start is now %d seconds %d microseconds\n", value, (long int) tv_prev_sample.tv_sec, (long int) tv_prev_sample.tv_usec);
			
		//now process bit 
		if (state == 0) {     //looking for preamble
			

			int new_bit = value;
			if (new_bit == 1) {
				preamble_count++;
			}
			else {
				preamble_count = 0;
			}
			
			
			if (preamble_count == 4) {
				state = 1;
				printf("preamble recognized! state is now %d\n", state);
				preamble_count = 0;
			}
			
		}
		
		else {
			printf("state is 1, revert to 0\n");
			state = 0;
		}
		
	
}

int main()
{
    int value;
    //int state;  //0 = looking for preamble  1 = reading the message  2 = looking for postamble  3 = message received
	struct node* pointer;
    gpio_init(gpio);
    gpio_direction(gpio, INPUT);
	create();
	int count = 0;

	
	struct itimerval timer;
	
	if (signal(SIGALRM, (void (*) (int)) get_next_bit) == SIG_ERR) {
		perror("Unable to catch SIGALRM");
		exit(1);
	}
	
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 10000;
	timer.it_interval = timer.it_value;
	if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
	
		perror("setitimer failed");
		exit(1);
	
	}
	
	//gettimeofday(&tv_prev_sample, NULL); //get previous timestamp
	while(1) {
	
		//printf("count is: %d\t queue size is %d\n", count, queuesize());
		
		pause();

		/*
		if (count==11)
		{
			ARRAY VERSION
			int k=0;
			for (k=0; k<12; k++)
			{
				printf("%d", recvd[k]);
				
			}
			printf("\n");
			
			display();
			deq();
		}
		*/
		
		
}


gpio_close(gpio);
return 0;
}

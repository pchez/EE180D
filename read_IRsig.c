//compile using gcc -lmraa -o read_IRsig.exe read_IRsig.c gpio.c
#include <stdio.h>
#include <unistd.h>
#include "gpio.h"
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>


int message[4];

struct timeval tv_prev_sample;
int preamble_count = 0;
int message_count = 0; 
int postamble_count = 0;
int error_count = 0; //only reset the file if the error count is > 1

int state = 0;  //0 = looking for preamble  1 = reading the message  2 = looking for postamble  3 = message received
int in_front_of_beacon = 0;

GPIO gpio = GPIO44;


int write_string_to_file(char s[]) {

	int fd;
	if ( (fd = open("./id", O_RDWR | O_TRUNC)) == -1) {
		fprintf(stderr, "error in opening");
		exit(1);
	}
	
	if (write(fd, s, 4) == -1) {
		fprintf(stderr, "error with writing\n");
		exit(1);
	}
	
	return 0;
	
}

//waits appropriate amount of time before sampling next value from GPIO input
//inputs are the timeval of the previous sampling time and the gpio
//returns bit read from GPIO input
void get_next_bit() {

	//struct timeval tv_now, tv_elapsed;
	int value;
	int fd;

		//gettimeofday(&tv_now, NULL);	//get new timestamp
		//timersub(&tv_now, &tv_prev_sample, &tv_elapsed); 	//compare new to prev
		
		//if ((long int)tv_elapsed.tv_usec >= 10000)	//if 10ms elapsed
		
			//printf("time elapsed = %d secs %d microsecs\t", (int) tv_elapsed.tv_sec, (int) tv_elapsed.tv_usec);
		value = !gpio_read(gpio);			//read bit
		fflush(stdout);
		//tv_prev_sample = tv_now;
		//printf("just read bit: %d\n", value);  //|  tv_start is now %d seconds %d microseconds\n", value, (long int) tv_prev_sample.tv_sec, (long int) tv_prev_sample.tv_usec);
			
		//now process bit 
		if (state == 0) {     //looking for preamble
			
			
			int new_bit = value;
			if (new_bit == 1) {
				preamble_count++;
			}
			else {
				preamble_count = 0;
				if (error_count < 1)
					error_count++;
				else {
				
					error_count = 0;
					if ((fd = open("./id", O_RDWR | O_TRUNC)) == -1) 
   						return 1; 
					(void) close(fd);
				
				}
				
			}
			
			
			if (preamble_count == 4) {
				state = 1;
				//printf("preamble recognized! state is now %d\n", state);
				preamble_count = 0;
			}
			
			
			
		}
		
		else if (state == 1) {
			//printf("state is 1, revert to 0\n");
			message[message_count] = value;
			message_count++;
			
			if (message_count == 4) {
				state++;
				message_count = 0;
			}
			//state = 0;
		}
		else {
		

			
			if (value == 0) 
				postamble_count++;
			else {

				postamble_count = 0;
				state = 0;
				if (error_count < 2)
					error_count++;
				else {
				
					error_count = 0;
					if ((fd = open("./id", O_RDWR | O_TRUNC)) == -1) 
   						return 1; 
					(void) close(fd);
				
				}
					
			}
			
			if (postamble_count == 4) {
				
				char id[4];
				memset(id, 0, 4*sizeof(char));
				state++;
				//printf("postamble recognized!\n");
				postamble_count = 0;
				
				//printf("message received: %d%d%d%d\n" , message[0], message[1], message[2], message[3]);
				sprintf(id, "%d%d%d%d",  message[0], message[1], message[2], message[3]);
				write_string_to_file(id);
				state = 0;
				error_count = 0;

			}
			
		}
		

		
		
	
}

int main()
{
    int value;
    //int state;  //0 = looking for preamble  1 = reading the message  2 = looking for postamble  3 = message received
	struct node* pointer;
    gpio_init(gpio);
    gpio_direction(gpio, INPUT);
	int count = 0;
	int fd;
	
	caddr_t result; 
	if ((fd = open("./id", O_RDWR | O_TRUNC)) == -1) 
   		return 1; 

	result = mmap(0, 10, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0); 
	
	(void) close(fd);

	
	struct itimerval timer;
	
	if (signal(SIGALRM, (void (*) (int)) get_next_bit) == SIG_ERR) {
		perror("Unable to catch SIGALRM");
		exit(1);
	}
	
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 50000;
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

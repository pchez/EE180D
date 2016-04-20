//compile using gcc -lmraa -o read_IRsig.exe read_IRsig.c gpio.c
#include <stdio.h>
#include "gpio.h"
#include <sys/time.h>
#include "queue.h"

int message[4];


int main()
{
    int value;
    struct timeval tv_prev, tv_new, tv_elapsed;
    GPIO gpio = GPIO44;
	struct node* pointer;
    gpio_init(gpio);
    gpio_direction(gpio, INPUT);
	create();
	int count = 0;
	while(1) {
	
		gettimeofday(&tv_prev, NULL); //get previous timestamp
		while (1)
		{
			gettimeofday(&tv_new, NULL);	//get new timestamp
			timersub(&tv_new, &tv_prev, &tv_elapsed); 	//compare new to prev
			
			
			if ((long int)tv_elapsed.tv_usec >= 10000)	//if 5ms elapsed
			{
				value = !gpio_read(gpio);			//read bit
				//recvd[count] = value;				//store bit
				enq(value);							//store bit in queue
				//printf("%d", value);
				fflush(stdout);
				
				if (queuesize() > 11)
				{
					count = 0;
					deq();
					pointer = front;
					
					while (pointer->info==1 && count < 4)	//if first 4 bits are 1
					{
						pointer = pointer->ptr;
						printf("bits 0-3 ");
						count++;
					}
					//printf("%d", count);
					if (count != 4)
					{
						break;
					}
					
					int idx=0;
					while (count >=4 && count < 8)
					{
						printf("bits 4-8 ");
						message[idx] = pointer->info;
						pointer = pointer->ptr;
						count++; idx++;
					}
					
					while (pointer->info==0 && (count>=8 && count < 12))
					{
						pointer = pointer->ptr;
						count++;
					}
					printf("seg fault \n");
					if (count != 12)
					{
						printf("count not 12");
						break;
					}
					
					//printf("message received: %d%d%d%d ", message[0], message[1], message[2], message[3]);
				}
				display();
				break;
			}
		}

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
		
		
		if (count > 11)
		{
			
			count = 0;
		}
}


gpio_close(gpio);
return 0;
}

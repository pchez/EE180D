#include <stdio.h>
#include "gpio.h"

int recvd[12];

int main()
{
    int value;
    GPIO gpio = GPIO44;

    gpio_init(gpio);
    gpio_direction(gpio, INPUT);

int index = 0;
while(1) {
    
    value = !gpio_read(gpio);
    recvd[index] = value;
    printf("%d", value);
    fflush(stdout);
    usleep(5000);
   


    index++;
    if (index > 11)
    {
	index = 0;

    }
	
}


gpio_close(gpio);
return 0;
}

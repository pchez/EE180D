#include <stdio.h>
#include "gpio.h"

int main()
{
    int value;
    GPIO gpio = GPIO44;

    gpio_init(gpio);
    gpio_direction(gpio, INPUT);

while(1) {
    value = gpio_read(gpio);
    printf("Value: %d\n", value);
}


gpio_close(gpio);
return 0;
}

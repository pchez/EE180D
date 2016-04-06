/*
 * gpio.c
 *
 * Author: In Hwan "Chris" Baek
 *	   chris.inhwan.baek@gmail.com
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "gpio.h"

void gpio_init(GPIO gpio)
{
	FILE* gpio_export;
	 
	gpio_export = fopen("/sys/class/gpio/export", "w");

	if (gpio_export == NULL) {
        	fprintf(stderr, "Failed to initialize: %s\n", strerror(errno));  
        	exit(1);  
	}
    
	fprintf(gpio_export, "%d", gpio);    
	fclose(gpio_export);
}

void gpio_close(GPIO gpio)
{
	FILE* gpio_unexport;
	 
	gpio_unexport = fopen("/sys/class/gpio/unexport", "w");
	if (gpio_unexport == NULL) {
        	fprintf(stderr, "Failed to unexport: %s\n", strerror(errno));  
        	exit(1);  
	}
    
	fprintf(gpio_unexport, "%d", gpio);    
	fclose(gpio_unexport);
}

void gpio_direction(GPIO gpio, DIRECTION direction)
{
	FILE* gpio_direction;
	char path_dir[MAX_BUF_SIZ];
	
	snprintf(path_dir, sizeof(path_dir), "/sys/class/gpio/gpio%d/direction", gpio);
	
	gpio_direction = fopen(path_dir, "w");
	
	if (gpio_direction == NULL) {
		fprintf(stderr, "Initialize GPIO first.\n");
		exit(1);
	}
	
	if (direction == OUTPUT) {
		fprintf(gpio_direction, "out");
	}
	else if (direction == INPUT) {
		fprintf(gpio_direction, "in");
	}
	else {
		fprintf(stderr, "Wrong direction type.\n");
		fclose(gpio_direction);
		exit(1);
	}
	fclose(gpio_direction);
}

void gpio_write(GPIO gpio, int value)
{
	FILE* gpio_value;
	char path_val[MAX_BUF_SIZ];
	
	snprintf(path_val, sizeof(path_val), "/sys/class/gpio/gpio%d/value", gpio);
	
	gpio_value = fopen(path_val, "w");
	fprintf(gpio_value, "%d", value);
	fclose(gpio_value);
}

int gpio_read(GPIO gpio)
{
	FILE* gpio_value;
	char path_val[MAX_BUF_SIZ], value_buf[MAX_BUF_SIZ];
	int value;
	
	snprintf(path_val, sizeof(path_val), "/sys/class/gpio/gpio%d/value", gpio);
	
	gpio_value = fopen(path_val, "r");
	fgets(value_buf, sizeof(value_buf), gpio_value);
	
	value = atoi(value_buf);
	fclose(gpio_value);
	return value;
}

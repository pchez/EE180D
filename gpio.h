/*
 * gpio.h
 *
 * Author: In Hwan "Chris" Baek
 *	   chris.inhwan.baek@gmail.com
 *
 */

#ifndef __GPIO_H__
#define __GPIO_H__

#define MAX_BUF_SIZ 64

typedef enum {
	GPIO44 = 44,
	GPIO45 = 45,
	GPIO46 = 46,
	GPIO47 = 47,
	GPIO48 = 48,
	GPIO49 = 49,
	GPIO15 = 15,
	GPIO14 = 14,
	GPIO183 = 183,
	GPIO182 = 182,
	GPIO12 = 12,
	GPIO13 = 13,
	PWM3 = 183,
	PWM2 = 182,
	PWM1 = 13,
	PWM0 = 12,
	GPIO131 = 131,
	GPIO130 = 130,
	GPIO129 = 129,
	GPIO128 = 128,
	TX = 131,
	RX = 130,
	RTS = 129,
	CTS = 128,
} GPIO;

typedef enum {
	INPUT,
	OUTPUT,
} DIRECTION;

void gpio_init(GPIO);
void gpio_close(GPIO);
void gpio_direction(GPIO, DIRECTION);
void gpio_write(GPIO, int);
int gpio_read(GPIO);

#endif // GPIO_H


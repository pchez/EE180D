#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <mraa/pwm.h>

#define MAXBUFSIZ 1024










int main()
{


	float brightness;
	char user_input[MAXBUFSIZ];
	

mraa_pwm_context pwm;
pwm = mraa_pwm_init(5);

	if (pwm==NULL)
	{
		fprintf(stderr, "Failed to initialize.\n");
		return 1;
	}
	mraa_pwm_period_us(pwm,26);
	mraa_pwm_enable(pwm,1);

	while(1)
	{
	printf("Enter brightness value (0-100): ");
	scanf("%s", user_input);
	brightness = atof(user_input);

        if (brightness > 100 || brightness < 0)
           printf("Error: Choose between 0 and 100\n");
        else {
            brightness = brightness/100;
            mraa_pwm_write(pwm, brightness);
        }
	
	}
	return 0;
}

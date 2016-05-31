//gcc -lmraa -lm -o upperbody4.exe upperbody4.c LSM9DS0.c

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <math.h>
#include <mraa/i2c.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include "LSM9DS0.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>

//globals
const int UPRIGHT = 0;
const int FACEUP = 1;
const int FACEDOWN = 2;
const int LEFT = 3;
const int RIGHT = 4;
const int SITTING = 5;
const int STANDING = 6;
const int ROTATE_LOWER_LEFT = 7;
const int ROTATE_LOWER_RIGHT = 8;
const int ROTATE_UPPER_LEFT = 9;
const int ROTATE_UPPER_RIGHT = 10;
const int ROTATE_ALL_LEFT = 11;
const int ROTATE_ALL_RIGHT = 12;
const int UNDEFINED = -1; //when patient is moving or in unknown position

int interval_posture, prev_posture_full, curr_posture_full = 0;
int sequence[3];
float prev_yaw_upper, curr_yaw_upper, prev_yaw_lower, curr_yaw_lower = 0.0;
float upper_rotation, lower_rotation = 0.0;
float delta_t = 0.1;
int fall_risk = 0;
int sensor_spike = 0; 	//bool for determining that if accel/gyro spikes then there is fall risk
char id[5];
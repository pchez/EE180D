//gcc -lmraa -lm -o upperbody4.exe upperbody4.c LSM9DS0.c
//don't forget to change IP address
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
int fall_risk = 0;

void error(char *msg)
{
    perror(msg);
    exit(0);
}

//concatenate two strings
char* concat(char *s1, char *s2)
{
	//malloc enough space for combination of strings
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the null byte

    //combine strings: copy s1 into result, then concatenate s2
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

int getPosture(data_t accel_data, float pitch_angle, float roll_angle)
{
    
    float pitch_threshold_upright, roll_threshold_left, roll_threshold_right, pitch_threshold_faceup, roll_threshold_faceup, pitch_threshold_facedown;
    int posture;
    float accel_data_x = accel_data.x;
    float accel_data_y = accel_data.y;
    float accel_data_z = accel_data.z;
    
    //define default orientations once sensor is attached to user to be as follows:
    //"Intel Edison" text facing upright, horizontal, flat on top of shoulder = sitting up, other orientations with respect to this
    
    //patient upright | pitch between -45 to 45, z < -0.7
    pitch_threshold_upright = 45; 
    
    //patient face up | pitch > 45, z > -0.5, roll between -45 and 45
    pitch_threshold_faceup = 45;  
    
    //patient face down | pitch < -45
    pitch_threshold_facedown = -45; 
    
    //patient turns to left side | roll < -45, z vector > -0.3
    roll_threshold_left =  -45; 
    
    //patient turns to right side | roll > 45, z vector > -0.3
    roll_threshold_right = 45;
    
     
    //Very crude right now. monitor all axes, compare with set values
    
    posture = UNDEFINED; //crude default
    
    if (abs(pitch_angle) < pitch_threshold_upright && accel_data_z < -0.7)
        posture = UPRIGHT;
    if (abs(roll_angle) < roll_threshold_right && accel_data_z > -0.5)
        posture = FACEUP;
    if (pitch_angle < pitch_threshold_facedown && abs(roll_angle) < roll_threshold_right)
        posture = FACEDOWN;
    if (roll_angle < roll_threshold_left && accel_data_z > -0.3)
        posture = RIGHT;
    if (roll_angle > roll_threshold_right)
        posture = LEFT;
    
    return posture;
    
}

int getFullPosture(int upper, int lower)
{
	int full_posture;
	if (upper==UPRIGHT)
	{
		if (lower==FACEDOWN)
			full_posture = STANDING;
		else
			full_posture = SITTING;
	}
	else if (upper==FACEUP)
	{
		if (lower==FACEDOWN)
			full_posture = STANDING;
		else
			full_posture = FACEUP;
	}
	else if (upper==FACEDOWN)
	{
		if (lower==FACEDOWN)
			full_posture = STANDING;
		else
			full_posture = FACEDOWN;
	}
	else if (upper==LEFT)
	{
		if (lower==UPRIGHT || lower==LEFT)
			full_posture = LEFT;
		if (lower==FACEDOWN)
			full_posture = STANDING;
		if (lower==RIGHT)
			full_posture = UNDEFINED;
	}
	else if (upper==RIGHT)
	{
		if (lower==UPRIGHT || lower==RIGHT)
			full_posture = RIGHT;
		if (lower==FACEDOWN)
			full_posture = STANDING;
		if (lower==LEFT)
			full_posture = UNDEFINED;
	}
	if (upper==UNDEFINED || lower==UNDEFINED)
	{
		full_posture = UNDEFINED;
	}
			
	
	return full_posture;
}

typedef enum {

    ACCEL_X,
    ACCEL_Y,
    ACCEL_Z,
    GYRO_X, 
    GYRO_Y,
    GYRO_Z, 
    MAG_X,
    MAG_Y, 
    MAG_Z,

} datatype_t;

typedef enum {

    X_DIR,
    Y_DIR,
    Z_DIR,
    POS,

} dir_t;


void printPostureString(int curr_posture_upper)
{
	char *posture_string;
	switch(curr_posture_upper)
	{
		case 0:
			posture_string = "upright";
			break;
		case 1:
			posture_string = "face up";
			break;
		case 2:
			posture_string = "face down";
			break;
		case 3:
			posture_string = "left";
			break;
		case 4:
			posture_string = "right";
			break;
		case 5:
			posture_string = "sitting";
			break;
		case 6:
			posture_string = "standing";
			break;
		default:
			posture_string = "undefined";
	}
        
    printf("%s\n", posture_string);
}


char* construct_message(dir_t dir, data_t accel_data, int curr_posture_upper) {

    char x_accel_value[30];
    char y_accel_value[30];
    char z_accel_value[30];
    char posture_value[30];
    char *full_message_x;
    char *full_message_y;
    char *full_message_z;
    char *full_message_posture;
    char message1[] = "{\"n\": \"";
    char message2[] = "\", \"v\": \"";
    char message3[] = "\"}";
    char *message4;
    char *message5;
    char *message6;

    if (dir == X_DIR) {

        snprintf(x_accel_value, 6, "%f", accel_data.x);
        message4 = concat(message1, "accel_x");
        message5 = concat(message4, message2);
        message6 = concat(message5, x_accel_value);
        full_message_x = concat(message6, message3);
        return full_message_x;

    }

    else if (dir == Y_DIR) {

        snprintf(y_accel_value, 6, "%f", accel_data.y);
        message4 = concat(message1, "accel_y");
        message5 = concat(message4, message2);
        message6 = concat(message5, y_accel_value);
        full_message_y = concat(message6, message3);
        return full_message_y;
    }

    else if (dir == Z_DIR) {
        
        snprintf(z_accel_value, 6, "%f", accel_data.z);
        message4 = concat(message1, "accel_z");
        message5 = concat(message4, message2);
        message6 = concat(message5, z_accel_value);
        full_message_z = concat(message6, message3);
        return full_message_z;

    }
    else if (dir == POS) {
    
        snprintf(posture_value, 6, "%d", curr_posture_upper);
        message4 = concat(message1, "posture_int");
        message5 = concat(message4, message2);
        message6 = concat(message5, posture_value);
        full_message_posture = concat(message6, message3);
    }
    else error("ERROR: invalid call to construct_message\n");


}

int getAngles(data_t accel_data, data_t gyro_data, data_t zero_rate, float *pitch_angle, float *roll_angle, float *yaw_angle)
{
    float accel_data_z;
    float accel_data_x;
    float accel_data_y;
    float gyro_data_x, gyro_rate_x;
    float gyro_data_y, gyro_rate_y;
    float gyro_data_z, gyro_rate_z;
    float gain = 90.0/15.0;
    
    accel_data_z = accel_data.z;
    if (accel_data_z > 1)
        accel_data_z = 1;
    if (accel_data_z < -1)
        accel_data_z = -1;
    
    accel_data_y = accel_data.y - 0.075;
    if (accel_data_y > 1)
        accel_data_y = 1;
    if (accel_data_y < -1)
        accel_data_y = -1;
    
    accel_data_x = accel_data.x + 0.085;
    if (accel_data_x > 1)
        accel_data_x = 1;
    if (accel_data_x < -1)
        accel_data_x = -1;
    
    
        
    *roll_angle = acos(accel_data_y/-1)*180/M_PI-90.0;
    *pitch_angle = acos(accel_data_x/-1)*180/M_PI-90.0;

	if (isMoving(gyro_data)==0)
	{
		gyro_rate_x = 0;
		gyro_rate_y = 0;
		gyro_rate_z = 0;
	}
	else
	{
		gyro_data_z = gyro_data.z;
		gyro_rate_z = (gyro_data_z - zero_rate.z)*gain;

	}
	
    *yaw_angle += gyro_rate_z*0.01;
    curr_yaw_upper = *yaw_angle;
    
    return 0;
}

void computeRotation(void)
{
	upper_rotation = curr_yaw_upper - prev_yaw_upper;
	lower_rotation = curr_yaw_lower - prev_yaw_lower;
	prev_yaw_upper = curr_yaw_upper;
	prev_yaw_lower = curr_yaw_lower;
	prev_posture_full = curr_posture_full;
}


int isMoving(data_t gyro_data)
{   
    float gyro_total = sqrt(pow(gyro_data.x, 2) + pow(gyro_data.y, 2) + pow(gyro_data.z, 2));
    if (gyro_total > 20.0)
    {
        return 1;
    }
    else
        return 0;
}

void determineFallRisk(int sequence0, int sequence1, int sequence2)
{
	if ((sequence0==FACEUP && sequence1==SITTING && sequence2==ROTATE_LOWER_LEFT)
	|| (sequence0==FACEUP && sequence1==SITTING && sequence2==ROTATE_ALL_LEFT)
	|| (sequence0==FACEUP && sequence1==SITTING && sequence2==ROTATE_LOWER_RIGHT)
	|| (sequence0==FACEUP && sequence1==SITTING && sequence2==ROTATE_ALL_RIGHT)
	|| (sequence1==LEFT && sequence2==SITTING)
	|| (sequence1==RIGHT && sequence2==SITTING))
	{
		fall_risk = 1;
		printf("FALL RISK\n");
	}
}

void getIntervalPosture()
{
	computeRotation();
	printf("upper rotation: %f ", upper_rotation);
	printf("lower rotation: %f ", lower_rotation);
	printf("current full posture: %d ", curr_posture_full);
	
	if (curr_posture_full==UNDEFINED)
		return;
	
	int i; 
	for (i=0; i<2; i++)
	{
		sequence[i] = sequence[i+1];	//rotate the array
	}
	
	if (upper_rotation > 60)				//upper body rotated right
	{
		sequence[2] = ROTATE_UPPER_RIGHT;
		if (lower_rotation > 60)			//lower body also rotated right
			sequence[2] = ROTATE_ALL_RIGHT;
	}
	else if (upper_rotation < -60)			//upper body rotated left
	{
		sequence[2] = ROTATE_UPPER_LEFT;
		if (lower_rotation < -60)			//lower body also rotated left
			sequence[2] = ROTATE_ALL_LEFT;
	}
	else if (lower_rotation > 60)			//only lower body rotated right
		sequence[2] = ROTATE_LOWER_RIGHT;
	else if (lower_rotation < -60)
		sequence[2] = ROTATE_LOWER_LEFT;	//only lower body rotated left
	else
		sequence[2] = curr_posture_full;	//if no significant, save current posture
	
	printf("current sequence: %d %d %d \n", sequence[0], sequence[1], sequence[2]);
	
	determineFallRisk(sequence[0], sequence[1], sequence[2]);

}


char* splitString(char* message)
{
	const char s[2] = ",";
	char *token;
	token = strtok(message, s);
	return strtok(NULL, s);
}

int main(int argc, char *argv[]) {

	/////VARIABLE DECLARATIONS/////

	//SENSORS
	mraa_i2c_context accel, gyro, mag;
	float a_res, g_res, m_res;
	data_t accel_data, gyro_data, mag_data;
	int16_t temperature;
    float pitch_angle, roll_angle, yaw_angle = 0;
    data_t zero_rate;
    char *x_accel_message;
    char *y_accel_message;
    char *z_accel_message;
    char *posture_message;
    int curr_posture_upper;
    int prev_posture_upper = 0;
	
	//SOCKETS AND MESSAGES
	int sockfd, newsockfd, sockfd2, newsockfd2, clilen, sockfd3; //Socket descriptor
    int portno, n, portno2, n2, portno3;
    char component[256];
    char endpoint[] = "127.0.0.1";
    struct sockaddr_in serv_addr, serv_addr2, cli_addr, gui_addr;
    struct hostent *server; //struct containing a bunch of info
    struct hostent *gui_server;
	char message_received[256];
	char message[256];

	//TIMING
	struct itimerval it_val;
	
	
	int count;

	/////SENSOR INITIALIZATION AND SETUP/////
	accel = accel_init();
	set_accel_scale(accel, A_SCALE_2G);
	set_accel_ODR(accel, A_ODR_100);
	a_res = calc_accel_res(A_SCALE_2G);

	gyro = gyro_init();
	set_gyro_scale(gyro, G_SCALE_245DPS);
	set_gyro_ODR(accel, G_ODR_190_BW_70);
	g_res = calc_gyro_res(G_SCALE_245DPS);

	mag = mag_init();
	set_mag_scale(mag, M_SCALE_2GS);
	set_mag_ODR(mag, M_ODR_125);
	m_res = calc_mag_res(M_SCALE_2GS);
	
	zero_rate = calc_gyro_offset(gyro, g_res);



	/////SOCKET SETUP FOR CLOUD/////
	/*
    portno = 41234;
    //create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); //create a new socket
    if (sockfd < 0) 
        error("ERROR opening socket");
    
    server = gethostbyname("127.0.0.1"); //takes a string like "www.yahoo.com", and returns a struct hostent which contains information, as IP address, address type, the length of the addresses...
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    
    //setup the server struct
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; //initialize server's address
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    
    //connect to server
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) //establish a connection to the server
        error("ERROR connecting");
	*/
	
	
	
	/////SOCKET SETUP FOR LOWER BODY EDISON/////
    portno2 = 2015;

    //create socket
    sockfd2 = socket(AF_INET, SOCK_STREAM, 0); //create a new socket
    if (sockfd2 < 0) 
        error("ERROR opening socket");
    
    //initialize socket structure
    bzero((char *) &serv_addr2, sizeof(serv_addr2));
   	serv_addr2.sin_family = AF_INET;
   	serv_addr2.sin_addr.s_addr = INADDR_ANY;
   	serv_addr2.sin_port = htons(portno2);
    
    //bind the host address
    if (bind(sockfd2, (struct sockaddr *) &serv_addr2, sizeof(serv_addr2)) < 0) {
      error("ERROR on binding");
   	}

	//start listening for 2nd edison
	listen(sockfd2, 5);
	clilen = sizeof(cli_addr);
	
	char* wait_for_lower_body = "Start the lower body sensor."; 
	printf("%s\n", wait_for_lower_body);
	
	newsockfd2 = accept(sockfd2, (struct sockaddr *)&cli_addr, &clilen);
	if (newsockfd2 < 0)
	{
		error("ERROR on accept");
	}

    ////SETUP FOR GUI CONNECTION////
    /*
    printf("setting up connection to gui... ");
    sleep(2);
    portno3 = 12000;

    sockfd3= socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd3 < 0)
        error("ERROR opening sockfd3");

    gui_server = gethostbyname("192.168.1.24");
    if (gui_server == NULL) {
        error("ERROR, no such host\n");
        exit(0);
    }

    memset((char*) &gui_addr, 0, sizeof(gui_addr));
    gui_addr.sin_family = AF_INET; 
    bcopy((char*)gui_server->h_addr, (char *)&gui_addr.sin_addr.s_addr, gui_server->h_length);
    gui_addr.sin_port = htons(portno3);

    if (connect(sockfd3, (struct sockaddr *)&gui_addr, sizeof(gui_addr)) < 0)
        error("ERROR connecting to gui");
    else 
        printf("Successfully connected to GUI!\n");
*/

    //first send message to lowerbody
    n = write(newsockfd2, "START", 6);
	if (n<0)
	   error("ERROR giving lower body the signal to start");


	//initialize posture sequence array
	sequence[0] = 0;
	sequence[1] = 0;
	sequence[2] = 0;


	//TIMER STUFF
	if (signal(SIGALRM, (void (*)(int))getIntervalPosture)==SIG_ERR)
	{
		perror("Unable to catch SIGALRM");
		exit(1);
	}

	it_val.it_value.tv_sec = 3;
	it_val.it_interval = it_val.it_value;
	if (setitimer(ITIMER_REAL, &it_val, NULL) == -1)
	{
		perror("error calling setitmer()");
		exit(1);
	}
	
    //read accel and gyro data 
    count = 0;
	while(1) {
        
		accel_data = read_accel(accel, a_res);
		gyro_data = read_gyro(gyro, g_res);
		mag_data = read_mag(mag, m_res);
		//temperature = read_temp(accel);
        
        
        ///GET OWN DATA
        getAngles(accel_data, gyro_data, zero_rate, &pitch_angle, &roll_angle, &yaw_angle);
        printf("upper moving: %d ", isMoving(gyro_data));
        printf("upper yaw angle: %f\n", yaw_angle);
        
        
        
		////COMMUNICATE WITH LOWER BODY EDISON/////
		//accept connection from lower body edison

		//if connection is established then start communication
		bzero(message_received, 256);
		n2 = read(newsockfd2, message_received, 255); //get lower body posture
		if (n2 < 0)
			error("Error reading from lower body edison");
			
		//split string, get lower body posture and current angle from lower body
		char* angle_received = splitString(message_received); 
		curr_yaw_lower = (float)atof(angle_received);
		//printPostureString(atoi(message_received));
		
		
        ////GET UPPER BODY POSTURE////
		if (isMoving(gyro_data)==0)        //if patient is stationary, calculate new posture
			curr_posture_upper = getPosture(accel_data, pitch_angle, roll_angle);
		else
			curr_posture_upper = UNDEFINED;    //else use undefined/transition state
		prev_posture_upper = curr_posture_upper; //set new value for prev posture
		
		
		////GET FULL BODY POSTURE////
		curr_posture_full = getFullPosture(curr_posture_upper, atoi(message_received));
		interval_posture = curr_posture_full;	//this is the variable to be stored into the current posture sequence (interval_posture is global)
		if (prev_posture_full==curr_posture_full) //if the posture in the previous element of the sequence is the same as the current posture
												  //update prev_posture_full in callback function
		{
			interval_posture = upper_rotation;
		} 	
		
		
        
		if (count == 3) {
            //send posture to cloud
            posture_message = construct_message(POS, accel_data, curr_posture_full);
        	
        	/*
        	/////COMMUNICATE WITH CLOUD/////
        	n = write(sockfd, posture_message, strlen(posture_message)); //write to the socket
    		if (n < 0) 
        		error("ERROR writing to cloud");
    		
            /////COMMUNICATE WITH GUI//////
            memset(send_to_gui, 0, 5*sizeof(char));
	    	snprintf(send_to_gui, 4, "%d", curr_posture_full);
            n = write(sockfd3, send_to_gui, strlen(send_to_gui));
	    if (n<0)
			error("ERROR communicating to GUI");
			*/
			
			
			
			//store accel data in string
            
            /*
			x_accel_message = construct_message(X_DIR, accel_data);

		    //printf("%s\n", full_message_x);

    		//send UDP message
    		n = write(sockfd,x_accel_message,strlen(x_accel_message)); //write to the socket
    		if (n < 0) 
        		error("ERROR writing to socket");

        	y_accel_message = construct_message(Y_DIR, accel_data);

		    //printf("%s\n", full_message_y);		

    		n = write(sockfd,y_accel_message,strlen(y_accel_message)); //write to the socket
    		if (n < 0) 
        		error("ERROR writing to socket");


            z_accel_message = construct_message(Z_DIR, accel_data);
		    //printf("%s\n", full_message_z);

    		n = write(sockfd,z_accel_message,strlen(z_accel_message)); //write to the socket
    		if (n < 0) 
        		error("ERROR writing to socket");
            */
        	count = 0;

		}
        
        
		//printf("X: %f\t Y: %f\t Z: %f\n", accel_data.x, accel_data.y, accel_data.z);
		//printf("X: %f\t Y: %f\t Z: %f\n", accel_data.x, accel_data.y, accel_data.z);
		//printf("\tX: %f\t Y: %f\t Z: %f\t||", gyro_data.x, gyro_data.y, gyro_data.z);
		//printf("\tX: %f\t Y: %f\t Z: %f\t||", mag_data.x, mag_data.y, mag_data.z);
		//printf("\t%ld\n", temperature);
		count++;
		usleep(10000);
        //printf("%i", loopcounter);

	}

	return 0;

}


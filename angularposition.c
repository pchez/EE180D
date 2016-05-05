//gcc -lmraa -lm -o angularposition.exe angularposition.c LSM9DS0.c
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
#include "LSM9DS0.h"

//globals
const int UPRIGHT = 0;
const int FACEUP = 1;
const int FACEDOWN = 2;
const int LEFT = 3;
const int RIGHT = 4;
const int UNDEFINED = -1;


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
        posture = LEFT;
    if (roll_angle > roll_threshold_right)
        posture = RIGHT;
    
    return posture;
    
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

char* construct_message(dir_t dir, data_t accel_data, int curr_posture) {

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
    int posture;

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
        snprintf(posture_value, 6, "%d", posture);
        message4 = concat(message1, "posture_int");
        message5 = concat(message4, message2);
        message6 = concat(message5, posture_value);
        full_message_posture = concat(message6, message3);
    }
    else error("ERROR: invalid call to construct_message\n");


}

int getAngles(data_t accel_data, data_t gyro_data, float *pitch_angle, float *roll_angle, float *yaw_angle)
{
    float accel_data_z;
    float accel_data_x;
    float accel_data_y;
    float gyro_data_x, gyro_rate_z;
    float gyro_data_y;
    float gyro_data_z;
    float offset = 2.5;
    float sensitivity = 5.5;
    
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
        
    *pitch_angle = acos(accel_data_y/-1)*180/M_PI-90.0;
    *roll_angle = acos(accel_data_x/-1)*180/M_PI-90.0;

	gyro_data_z = gyro_data.z;
	gyro_rate_z = (gyro_data_z + 3.05)*sensitivity;
    *yaw_angle += gyro_rate_z*0.01;
    usleep(1000);
    
    
    
    return 0;
}

int isMoving(data_t gyro_data, float prev_gyro_x, float prev_gyro_y, float prev_gyro_z)
{   
	
    float gyro_total = sqrt(pow((gyro_data.x-prev_gyro_x), 2) + pow((gyro_data.y-prev_gyro_y), 2) + pow((gyro_data.z-prev_gyro_z), 2));
    //printf("gyro total: %f\n", gyro_total);
    if (gyro_total > 5.0)
    {
        return 1;
    }
    else
        return 0;
}

int main(int argc, char *argv[]) {

	/////VARIABLE DECLARATIONS/////

	//SENSORS
	mraa_i2c_context accel, gyro, mag;
	float a_res, g_res, m_res;
	data_t accel_data, gyro_data, mag_data;
	int16_t temperature;
    float pitch_angle, roll_angle, yaw_angle;
    char *x_accel_message;
    char *y_accel_message;
    char *z_accel_message;
    char *posture_message;
    int curr_posture;
    int prev_posture = 0;
    float curr_gyro_x, curr_gyro_y, curr_gyro_z;
    float prev_gyro_x, prev_gyro_y, prev_gyro_z;
	
	//SOCKETS AND MESSAGES
	int sockfd; //Socket descriptor
    int portno, n;
    char component[256];
    char endpoint[] = "127.0.0.1";
    struct sockaddr_in serv_addr;
    struct hostent *server; //struct containing a bunch of info
	char message[256];

	int count;

	/////SENSOR INITIALIZATION AND SETUP
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


    //read accel and gyro data 
    count = 0;
    prev_gyro_x = 0;
    prev_gyro_y = 0;
    prev_gyro_z = 0;
	while(1) {
        
		accel_data = read_accel(accel, a_res);
		gyro_data = read_gyro(gyro, g_res);
		//mag_data = read_mag(mag, m_res);
		//temperature = read_temp(accel);
        
		curr_gyro_x = gyro_data.x;
		curr_gyro_y = gyro_data.y;
		curr_gyro_z = gyro_data.z;
		
        getAngles(accel_data, gyro_data, &pitch_angle, &roll_angle, &yaw_angle);
        printf("is moving: %d\n", isMoving(gyro_data, prev_gyro_x, prev_gyro_y, prev_gyro_z) );
        //printf("roll angle: %f ", roll_angle);
        //printf("pitch angle: %f ", pitch_angle);
        //printf("z vector: %f\n", accel_data.z);
        
        
		if (count == 3) {
            //send posture to cloud
            
            if (isMoving(gyro_data, prev_gyro_x, prev_gyro_y, prev_gyro_z)==0)        //if patient is stationary, calculate new posture
                curr_posture = getPosture(accel_data, pitch_angle, roll_angle);
            else
                curr_posture = prev_posture;    //else just use the old posture
            if (curr_posture==UNDEFINED)
                curr_posture = prev_posture;
            prev_posture = curr_posture; //set new value for prev posture
            
            posture_message = construct_message(POS, accel_data, curr_posture);
            //n = write(sockfd, posture_message, strlen(posture_message)); //write to the socket
    		//if (n < 0) 
        	//	error("ERROR writing to socket");
    
        	count = 0;

		}
        
        prev_gyro_x = curr_gyro_x;
        prev_gyro_y = curr_gyro_y;
        prev_gyro_z = curr_gyro_z;
        
        char *posture_string;
        switch(curr_posture)
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
            default:
                posture_string = "undefined";
        }
        
        //printf("current orientation: %s \n", posture_string);
		//printf("X: %f\t Y: %f\t Z: %f\n", accel_data.x, accel_data.y, accel_data.z);
		//printf("X: %f\t Y: %f\t Z: %f\n", accel_data.x, accel_data.y, accel_data.z);
		//printf("\tX: %f\t Y: %f\t Z: %f\t\n", gyro_data.x, gyro_data.y, gyro_data.z);
		//printf("yaw angle: %f\n", yaw_angle);
		//printf("\tX: %f\t Y: %f\t Z: %f\t||", mag_data.x, mag_data.y, mag_data.z);
		//printf("\t%ld\n", temperature);
		
		count++;
		usleep(100000);

	}

	return 0;

}




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

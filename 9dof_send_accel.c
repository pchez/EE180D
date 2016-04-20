#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <mraa/i2c.h>
#include "LSM9DS0.h"

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

} dir_t;

char* construct_accel_message(dir_t dir, data_t accel_data) {


    char x_accel_value[30];
    char y_accel_value[30];
    char z_accel_value[30];
    char *full_message_x;
    char *full_message_y;
    char *full_message_z;
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
    else error("ERROR: invalid call to construct_accel_message\n");


}

int main(int argc, char *argv[]) {

	/////VARIABLE DECLARATIONS/////

	//SENSORS
	mraa_i2c_context accel, gyro, mag;
	float a_res, g_res, m_res;
	data_t accel_data, gyro_data, mag_data;
	int16_t temperature;
    char *x_accel_message;
    char *y_accel_message;
    char *z_accel_message;
	
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

	/////SOCKET SETUP/////
	//resolve arguments from command line
	//if (argc < 2) {
    //   fprintf(stderr,"Error: need argument (component)\n");
    //   exit(0);
    //}
    //store arguments into variables
    //strcpy(component, argv[1]); 
    
    //this is the port number that the edison reserves for the cloud?
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


    //lets only read acceleration form now
    count = 0;
	while(1) {
		accel_data = read_accel(accel, a_res);
		//gyro_data = read_gyro(gyro, g_res);
		//mag_data = read_mag(mag, m_res);
		//temperature = read_temp(accel);

		if (count == 3) {
			//store accel data in string
			x_accel_message = construct_accel_message(X_DIR, accel_data);

		    //printf("%s\n", full_message_x);

    		//send UDP message
    		n = write(sockfd,x_accel_message,strlen(x_accel_message)); //write to the socket
    		if (n < 0) 
        		error("ERROR writing to socket");

        	y_accel_message = construct_accel_message(Y_DIR, accel_data);

		    //printf("%s\n", full_message_y);		

    		n = write(sockfd,y_accel_message,strlen(y_accel_message)); //write to the socket
    		if (n < 0) 
        		error("ERROR writing to socket");


            z_accel_message = construct_accel_message(Z_DIR, accel_data);
		    //printf("%s\n", full_message_z);

    		n = write(sockfd,z_accel_message,strlen(z_accel_message)); //write to the socket
    		if (n < 0) 
        		error("ERROR writing to socket");

        	count = 0;

		}
		printf("X: %f\t Y: %f\t Z: %f\n", accel_data.x, accel_data.y, accel_data.z);
		//printf("\tX: %f\t Y: %f\t Z: %f\t||", gyro_data.x, gyro_data.y, gyro_data.z);
		//printf("\tX: %f\t Y: %f\t Z: %f\t||", mag_data.x, mag_data.y, mag_data.z);
		//printf("\t%ld\n", temperature);
		count++;
		usleep(100000);

	}

	return 0;

}

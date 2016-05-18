//compile using gcc -o test_read_IR.exe test_read_IR.c
//needs to be in the same folder as the file "id"
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>


int main()
{
	int write = 0;  //if 0, dont write to file. if 1, write to file.
	char message[5];
	memset(message, 0, sizeof(char) * 11);
	int fd;
	int n;
	 
	caddr_t result; 
	if ((fd = open("./id", O_RDWR | O_TRUNC)) == -1) 
   		return ((caddr_t)-1); 

	result = mmap(0, 10, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0); 
	
	(void) close(fd);
	
	while(1) {
	
		memset(message, 0, sizeof(char) * 11);
		
		if ((fd = open("./id", O_RDWR)) == -1) 
   			return ((caddr_t)-1); 
				
		if ((n = read(fd, message, 4)) == -1) {
			fprintf(stderr, "error with reading: %s\n", strerror(errno));
			exit(1);
		}

		if (n == 0)
			printf("not in front of LED\n");
			
		else  {
		
			printf("we're in front of the IR LED! The id is %s\n", message);
		
		}		

		close(fd);
		usleep(50000);

		//pause();

		
	}

return 0;
}
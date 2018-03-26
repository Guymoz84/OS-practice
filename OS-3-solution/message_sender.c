#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "message_slot.h"

int main(int argc,char** argv){
    int fd;
    int check=0;
    if(argc!=4)
    {
        printf("Number of arguments is invalid!\n");
        exit(-1);
    }
    fd=open(argv[1],O_RDWR);//first stage - open
    if(fd<0)
    {
        printf ("Can't open device file, error is: %s\n",strerror(errno));
        exit(-1);
    }
    check=ioctl(fd,MSG_SLOT_CHANNEL,atoi(argv[2]));//second stage
    if(check<0){
    	printf("Fail when calling ioctl function,error is: %s\n",strerror(errno));
    	exit(1);
    }
    check=write(fd,argv[3],strlen(argv[3])+1);//third stage
    if(check<0){
    	printf("Fail when calling write function,error is: %s\n",strerror(errno));
    	exit(1);
    }
    check=close(fd);//fourth stage
    if(check<0){
    	printf("Fail to close file,error is: %s\n",strerror(errno));
    	exit(1);
    }
    printf("The status is: Passed!\n");//Fifth stage
    exit(0);//Exit value should be 0 on success and a non-zero value on error
}

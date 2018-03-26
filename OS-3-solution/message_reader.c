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
    char buf_read[BUF_LEN];
    int check=0;
    int i=0;
    if(argc!=3){
        printf("Number of arguments is invalid!\n");
        exit(-1);
    }
    fd=open(argv[1],O_RDWR);//first stage - open
    if(fd<0)
    {
        printf("Can't open device file, error is: %s\n",strerror(errno));
        exit(-1);
    }
    check=ioctl(fd,MSG_SLOT_CHANNEL,atoi(argv[2]));//second stage
    if(check<0){
    	printf("Fail when calling ioctl function, error is: %s\n",strerror(errno));
    	exit(1);
    }
    check=read(fd,buf_read,BUF_LEN);//third stage
    if(check<0){
    	printf("Fail when calling read function, error is: %s\n",strerror(errno));
    	exit(1);
    }
    buf_read[check-1]='\0';
    check=close(fd);
    if(check<0){//fourth stage
    	printf("Fail to close file, error is: %s\n",strerror(errno));
    	exit(1);
    }
    printf("The message is:%s\n",buf_read);//fifth stage
    printf("The status is: Passed!\n");//Fifth stage
    exit(0);//Exit value should be 0 on success and a non-zero value on error
}

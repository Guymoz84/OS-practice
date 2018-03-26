#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define BUF_LEN 1024
#define STR_LEN 11 //2^32 -1 -> max length of input of string is 10, +1-> null char

unsigned int sendAndReceiveData(int socket_id,unsigned int length);
void closeSockFd(int socket_id,int fd);

int main(int argc,char** argv){
	unsigned int length;
	unsigned int num_char_printable=0;
	int socket_id=0;
	int check=0;
	struct addrinfo* addr;
	if (argc!=4){// Need to get 4 arguments(including the file)
		printf("Number of arguments is not 3\n");
		exit(1);
	}
	if(strlen(argv[3])>10){
		printf("Length argument is invalid\n");
		exit(1);
	}
	if((socket_id=socket(AF_INET,SOCK_STREAM,0))<0){
		printf("Failed to create a new socket_id, the error is: %s\n", strerror(errno));
		exit(1);
	}
	length=(unsigned int)atol(argv[3]);
	if(getaddrinfo(argv[1],argv[2],NULL,&addr)!=0){//take care of the host name
		printf("Failed to call getaddrinfo, the error is: %s\n", strerror(errno));
		if(close(socket_id)<0){
			printf("Failed to close socket_id, the error is: %s\n", strerror(errno));
		}
		exit(1);
	}
	while(addr!=NULL){//connect to socket_id
		check=connect(socket_id,addr->ai_addr,addr->ai_addrlen);
		if(check==0){
			break;
		}
		addr=addr->ai_next;
	}
	if(addr==NULL){
		printf("Failed to connect to the new socket_id at client side, the error is: %s\n", strerror(errno));
		if(close(socket_id)<0){
			printf("Failed to close socket_id, the error is: %s\n", strerror(errno));
		}
		exit(1);
	}
	num_char_printable=sendAndReceiveData(socket_id,length);//send data so server side
	printf("# of printable characters: %u\n",num_char_printable);//as wished
	exit(0);
}

unsigned int sendAndReceiveData(int socket_id,unsigned int length){
	char buffer[BUF_LEN];
	char str_length[STR_LEN];
	unsigned int tot_read=0;
	int fd=0;
	int bytes_read=0;
	sprintf(str_length,"%u",length); //convert the input length to use as a string(char*)
	memset(buffer,0,BUF_LEN);
	fd=open("/dev/urandom",O_RDONLY);
	if(fd<0){
		printf("Failed to open /dev/urandom, the error: %s\n", strerror(errno));
		if(close(socket_id)<0){
			printf("Failed to close socket_id, the error is: %s\n", strerror(errno));
		}
		exit(1);
	}
	if(send(socket_id,str_length,STR_LEN,0)<0){
		printf("Failed to send data, the error is: %s\n", strerror(errno));
		if(close(socket_id)<0){
			printf("Failed to close socket_id, the error is: %s\n", strerror(errno));
		}
		exit(1);
	}
	while(tot_read<length){
		if(length-tot_read<BUF_LEN){
			bytes_read=read(fd,buffer,length-tot_read);
		}
		else{
			bytes_read=read(fd,buffer,BUF_LEN);
		}
		if(bytes_read<0){
			printf("Failed to read data from /dev/urandom, the error: %s\n", strerror(errno));
			closeSockFd(socket_id,fd);
			exit(1);
		}
		if(send(socket_id,buffer,bytes_read,0)<0){//sending data to server side
			printf("Failed to send data, the error is: %s\n", strerror(errno));
			closeSockFd(socket_id,fd);
			exit(1);
		}
		tot_read+=bytes_read;
		memset(buffer,0,BUF_LEN);//re-setting the buffer
	}
	if(recv(socket_id,buffer,BUF_LEN,0)<0){//receiving data from server side about the number of printable characters
		printf("Failed to receiving data, the error is: %s\n", strerror(errno));
		closeSockFd(socket_id,fd);
		exit(1);
	}
	if(close(fd)<0){
		printf("Failed to close fd of /dev/urandom, the error is: %s\n", strerror(errno));
	}
	return ((unsigned int)atol(buffer));
}
void closeSockFd(int socket_id,int fd){
	if(close(socket_id)<0){
		printf("Failed to close socket_id, the error is: %s\n", strerror(errno));
	}
	if(close(fd)<0){
		printf("Failed to close fd of /dev/urandom, the error is: %s\n", strerror(errno));
	}
}

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUF_LEN 1024
#define STR_LEN 11 //2^32 -1 -> max length ofinput of stringis 10, +1-> null char
#define MAX_CHAR 126
#define MIN_CHAR 32
#define NUMBER_PRINTABLE_CHARS 95

unsigned int num_threads_running=0;
int socket_id=0;
unsigned int printable_chars[NUMBER_PRINTABLE_CHARS];//global array for counting printable chars

pthread_mutex_t lock;
pthread_mutex_t temp_lock;

///"CREDIT" - http://beej.us/net2/bgnet.html#theory
///"CREDIT" - http://beej.us/net2/bgnet.html#socket

void* talkWithClient(void* temp_id);//connection with client
void sigintHandler(int sigint_num);//ready to get sigint
void closeAll(int socket_id,int current_id);//close sockets

int main(int argc,char** argv){
	struct sockaddr_in addr1;
	struct sockaddr_in addr2;
	socklen_t addr_size;
	pthread_t thread;
	long temp_id=0;
	if(argc!=2){
		printf("Number of arguments is not 1\n");
		exit(1);
	}
	if(pthread_mutex_init(&lock,NULL)!=0){
		printf("Failed to init the main lock,the error is: %s\n",strerror(errno));
		exit(1);
	}
	if(pthread_mutex_init(&temp_lock,NULL)!=0){
		printf("Failed to init the temp lock,the error is: %s\n",strerror(errno));
		exit(1);
	}
	if((socket_id=socket(AF_INET,SOCK_STREAM,0))<0){
		printf("Failed to create a new socket_id, the error is: %s\n", strerror(errno));
		exit(1);
	}
	if(signal(SIGINT,sigintHandler)==SIG_ERR){//ready to get sigint and take face
		printf("Failed to call sigint handler,the error is: %s\n",strerror(errno));
		if(close(socket_id)<0){
			printf("Failed to close socket_id,the error is: %s\n",strerror(errno));
		}
		exit(1);
	}
	memset(&addr1,0,sizeof(addr1));
	addr1.sin_family=AF_INET;
	addr1.sin_port=htons(atoi(argv[1]));
	addr1.sin_addr.s_addr=htonl(INADDR_ANY);
	addr_size=sizeof(struct sockaddr_in);

	if(bind(socket_id,(struct sockaddr*)&addr1,addr_size)!=0){//bind the socket
		printf("Failed to bind socket,the error is: %s\n",strerror(errno));
		if(close(socket_id)<0){
			printf("Failed to close socket_id,the error is: %s\n",strerror(errno));
		}
		exit(1);
	}
	if(listen(socket_id,84)<0){//socket can now listen to "clients"
		printf("Failed to listen to socket,the error is: %s\n",strerror(errno));
		if(close(socket_id)<0){
			printf("Failed to close socket_id,the error is: %s\n",strerror(errno));
		}
		exit(1);
	}
	memset(printable_chars,0,(NUMBER_PRINTABLE_CHARS)*sizeof(unsigned int));
	while(1){
		temp_id=accept(socket_id,(struct sockaddr*)&addr2,&addr_size);//connection with a new client
		if(temp_id<0){
			printf("Failed to accept socket,the error is: %s\n",strerror(errno));
			if(close(socket_id)<0){
				printf("Failed to close socket_id,the error is: %s\n",strerror(errno));
			}
			exit(1);
		}
		if(pthread_mutex_lock(&lock)!=0){
			printf("Failed to grab the main lock,the error is: %s\n",strerror(errno));
			closeAll(socket_id,temp_id);
			exit(1);
		}
		num_threads_running++;//count number of threads that are runnning
		if(pthread_mutex_unlock(&lock)!=0){
			printf("Failed to unlock main lock,the error is: %s\n",strerror(errno));
			closeAll(socket_id,temp_id);
			exit(1);
		}
		if(pthread_create(&thread,NULL,talkWithClient,(void*)(temp_id))!=0){
			printf("Failed to create a new thread,the error is: %s\n",strerror(errno));
			closeAll(socket_id,temp_id);
			exit(1);
		}
	}
	if(pthread_mutex_destroy(&lock)!=0){
		printf("Failed to destroy the main lock,the error is: %s\n",strerror(errno));
		exit(1);
	}
	if(pthread_mutex_destroy(&temp_lock)!=0){
		printf("Failed to destroy the temp lock,the error is: %s\n",strerror(errno));
		exit(1);
	}
	if(close(socket_id)<0){
		printf("Failed to close socket_id,the error is: %s\n",strerror(errno));
	}
	if(close(temp_id)<0){
		printf("Failed to close temp_id,the error is: %s\n",strerror(errno));
	}
	return 0;
}

void* talkWithClient(void* temp_id){
	char buffer[BUF_LEN];
	unsigned int tmp_printable_chars[NUMBER_PRINTABLE_CHARS];
	unsigned int tot_read=0;
	unsigned int length=0;
	unsigned int tot_print_chars=0;
	int current_id=(intptr_t)temp_id;
	int i=0;
	int bytes_read=0;
	int bytes_to_read=0;
	memset(buffer,0,BUF_LEN);
	memset(tmp_printable_chars,0,(NUMBER_PRINTABLE_CHARS)*sizeof(unsigned int));
	if(recv(current_id,buffer,STR_LEN,0)<0){//receive the number of bytes that the user asked for(char*)
		printf("Failed to receive data,the error is: %s\n",strerror(errno));
		closeAll(socket_id,current_id);
		exit(1);
	}
	length=(unsigned int)atol(buffer);//the length(number)
	memset(buffer,0,BUF_LEN);
	while(tot_read<length){
		if((bytes_to_read=length - tot_read) >= BUF_LEN){
			bytes_to_read=BUF_LEN;
		}
		if((bytes_read=recv(current_id,buffer,bytes_to_read,0))<0){
			printf("Failed to receive data,the error is: %s\n",strerror(errno));
			closeAll(socket_id,current_id);
			exit(1);
		}
		tot_read+=bytes_read;
		for(i=0;i<bytes_read;i++){
			if(buffer[i]>=MIN_CHAR && buffer[i]<=MAX_CHAR){//update temp array
				tmp_printable_chars[buffer[i]-MIN_CHAR]++;
				tot_print_chars++;
			}
		}
		memset(buffer,0,BUF_LEN);
	}

	if(pthread_mutex_lock(&temp_lock)!=0){
		printf("Failed to grab the temp lock,the error is: %s\n",strerror(errno));
		closeAll(socket_id,current_id);
		exit(1);
	}
	for(i=0;i<NUMBER_PRINTABLE_CHARS;i++){
		printable_chars[i]+=tmp_printable_chars[i];//update global array
	}
	if(pthread_mutex_unlock(&temp_lock)!=0){
		printf("Failed to unlock the temp lock,the error is: %s\n",strerror(errno));
		closeAll(socket_id,current_id);
		exit(1);
	}
	sprintf(buffer,"%u",tot_print_chars);
	if(send(current_id,buffer,strlen(buffer),0)<0){
		printf("Failed to send data to client,the error is: %s\n",strerror(errno));
		closeAll(socket_id,current_id);
		exit(1);
	}
	if(pthread_mutex_lock(&lock)!=0){
		printf("Failed to grab the main lock,the error is: %s\n",strerror(errno));
		closeAll(socket_id,current_id);
		exit(1);
	}
	num_threads_running--;//thread has finished
	if(pthread_mutex_unlock(&lock)!=0){
		printf("Failed to unlock the main lock,the error is: %s\n",strerror(errno));
		closeAll(socket_id,current_id);
		exit(1);
	}
	pthread_exit(NULL);
}

void sigintHandler(int sigint_num){
	int i=0;
	if(close(socket_id)<0){
		printf("Failed to close socket_id,the error is: %s\n",strerror(errno));
	}
	while(num_threads_running!=0){
		sleep(0);
	}
	printf("\n");
	for(i=0;i<NUMBER_PRINTABLE_CHARS;i++){
		printf("char '%c' : %u times\n",(char)(i + MIN_CHAR),printable_chars[i]);
	}
	exit(0);
}

void closeAll(int socket_id,int current_id){
	if(close(socket_id)<0){
		printf("Failed to close socket,the error is: %s\n",strerror(errno));
	}
	if(close(current_id)<0){
		printf("Failed to close socket,the error is: %s\n",strerror(errno));
	}
}

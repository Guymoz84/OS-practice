
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>

#define BUF_LEN 1048576//1024*1024

char** input_files;
char output_buff[BUF_LEN]={0};
int output_fd=0;
int output_size=0;
int running=0;
int num_threads_running=0;
int max_length=0;

pthread_mutex_t main_lock;
pthread_mutex_t output_lock;
pthread_cond_t done;
pthread_cond_t rounds;

void init();//init locks and conditions
void* readerThread(void* t);//as in orders
void updateRunning();//grab and throw the lock when running
void opAndContinue(char* buffer,int num_bytes_read);//xoring data and update by number of threads running
void stillRunning();
void lastThreadRunning();//special case for the last thread
int readInput(int input_file_fd, char* buffer,int buffer_size);//read chunk from an input file
int writeOutput();//write to output file
void lockAndUnlock();
void destroyExit();//destroy all locks and conditions and exit

int main(int argc,char** argv){
	long i=0;
	int check=0;
	if(argc<3){
		printf("Error: number of arguments is less then 2 files..\n");
		exit(1);
	}
	input_files=argv;
	pthread_t threads[argc-2];
	num_threads_running=argc-2;
	printf("Hello, creating %s from %d input files\n",argv[1],argc-2);
	init();
	output_fd=open(argv[1],O_WRONLY|O_TRUNC|O_CREAT,S_IRWXU);
	if(output_fd<0){
		printf("Error: failed to open the output file\n");
		exit(1);
	}
	check=pthread_mutex_lock(&main_lock);
	if(check!=0){
		printf("Error: failed to grab main lock\n");
		exit(1);
	}
	for(i=0;i<(argc-2);i++){
		check=pthread_create(&(threads[i]),NULL,readerThread,(void*)(i));
		if(check!=0){
			printf("Error: failed to create thread\n");
			exit(1);
		}
		running++;
	}
	check=pthread_mutex_unlock(&main_lock);
	if(check!=0){
		printf("Error: failed to unlock main lock\n");
		exit(1);
	}
	lockAndUnlock();
	check=close(output_fd);
	if(check<0){
		printf("Error: failed to close the output file\n");
		exit(1);
	}
    printf("Created %s with size %d bytes\n",argv[1],output_size);
	destroyExit();
}
void* readerThread(void* t){
	char buffer[BUF_LEN]={0};
	int i=(intptr_t)t;
	int active_reader=1;
	int num_bytes_read=0;
	int check=0;
	int input_file=open(input_files[i+2],O_RDONLY);
	if(input_file<0){
		printf("Error: failed to open the input file: %s\n",input_files[i+2]);
		exit(1);
	}
	while(active_reader!=0){
		memset(buffer,0,BUF_LEN);
		num_bytes_read=readInput(input_file,buffer,BUF_LEN);
		check=pthread_mutex_lock(&output_lock);
		if(check!=0){
			printf("Error: failed to grab semi lock\n");
			exit(1);
		}
		if(num_bytes_read<BUF_LEN){
			active_reader=0;
			updateRunning();
		}
		opAndContinue(buffer,num_bytes_read);
	}
	check=pthread_mutex_lock(&main_lock);
	if(check!=0){
		printf("Error: failed to grab main lock\n");
		exit(1);
	}
	if(running==0){
		pthread_cond_signal(&done);
	}
	check=pthread_mutex_unlock(&main_lock);
	if(check!=0){
		printf("Error: failed to unlock main lock\n");
		exit(1);
	}
	pthread_exit(NULL);
}
void init(){
	int check=0;
	check=pthread_mutex_init(&main_lock,NULL);
	if(check!=0){
		printf("Error: failed to init the main lock\n");
		exit(1);
	}
	check=pthread_mutex_init(&output_lock,NULL);
	if(check!=0){
		printf("Error: failed to init the output lock\n");
		exit(1);
	}
	check=pthread_cond_init (&done,NULL);
	if(check!=0){
		printf("Error: failed to init the done condition\n");
		exit(1);
	}
	check=pthread_cond_init (&rounds,NULL);
	if(check!=0){
		printf("Error: failed to init the rounds condition\n");
		exit(1);
	}
}
void updateRunning(){
	int check=0;
	check=pthread_mutex_lock(&main_lock);
	if(check!=0){
		printf("Error: failed to grab the main lock\n");
		exit(1);
	}
	running--;
	check=pthread_mutex_unlock(&main_lock);
	if(check!=0){
		printf("Error: failed to unlock the main lock\n");
		exit(1);
	}
}
void opAndContinue(char* buffer,int num_bytes_read){
	int i=0;
	for(i=0;i<BUF_LEN;i++){
		output_buff[i]^=buffer[i];
	}
	if(num_bytes_read>max_length){
		max_length=num_bytes_read;
	}
	if(num_threads_running!=1){
		stillRunning();
	}
	else{
		lastThreadRunning();
	}
}
void stillRunning(){
	int check=0;
	check=pthread_mutex_lock(&main_lock);
	if(check!=0){
		printf("Error: failed to grab the main lock\n");
		exit(1);
	}
	num_threads_running--;
	check=pthread_mutex_unlock(&output_lock);
	if(check!=0){
		printf("Error: failed to unlock the semi lock\n");
		exit(1);
	}
	check=pthread_cond_wait(&rounds,&main_lock);
	if(check!=0){
		printf("Error: failed at cond wait pthread call\n");
		exit(1);
	}
	check=pthread_mutex_unlock(&main_lock);
	if(check!=0){
		printf("Error: failed to unlock the main lock\n");
		exit(1);
	}
}
void lastThreadRunning(){
	int check=0;
	writeOutput();
	output_size+=max_length;
	max_length=0;
	memset(output_buff,0,BUF_LEN);
	check=pthread_mutex_lock(&main_lock);
	if(check!=0){
		printf("Error: failed to grab the main lock\n");
		exit(1);
	}
	num_threads_running=running;
	check=pthread_cond_broadcast(&rounds);
	if(check!=0){
		printf("Error: failed to call broadcast pthread call\n");
		exit(1);
	}
	check=pthread_mutex_unlock(&main_lock);
	if(check!=0){
		printf("Error: failed to unlock the main lock\n");
		exit(1);
	}
	check=pthread_mutex_unlock(&output_lock);
	if(check!=0){
		printf("Error: failed to unlock the semi lock\n");
		exit(1);
	}
}
int readInput(int input_file_fd,char* buffer,int buffer_size){
	char* tmp_buf=buffer;
	int bytes_read=0;
	int total_read_file=0;
	int tmp_size=buffer_size;
	while((bytes_read=read(input_file_fd,tmp_buf,tmp_size))!=0){
		if(bytes_read<0){
			printf("failed to read from the input file!\n");
			exit(1);
		}
		tmp_size-=bytes_read;
		tmp_buf+=bytes_read;
		total_read_file+=bytes_read;
	}
	return total_read_file;
}
int writeOutput(){
	char* tmp_buf=output_buff;
	int bytes_write=0;
	int total=0;
	int tmp_size=max_length;
	while((bytes_write=write(output_fd,tmp_buf,tmp_size))!=0){
		if(bytes_write<0){
			printf("Error: failed to write to the output file\n");
			exit(1);
		}
		tmp_size-=bytes_write;
		tmp_buf+=bytes_write;
		total+=bytes_write;
	}
	return total;
}
void lockAndUnlock(){
	int check=0;
	check=pthread_mutex_lock(&main_lock);
	if(check!=0){
		printf("Error: failed to grab the main lock\n");
		exit(1);
	}
	check=pthread_cond_wait(&done,&main_lock);
	if(check!=0){
		printf("Error: failed at cond wait pthread call\n");
		exit(1);
	}
	check=pthread_mutex_unlock(&main_lock);
	if(check!=0){
		printf("Error: failed to unlock the main lock\n");
		exit(1);
	}
}
void destroyExit(){
	pthread_mutex_destroy(&main_lock);
	pthread_cond_destroy(&done);
	pthread_mutex_destroy(&output_lock);
	pthread_cond_destroy(&rounds);
	pthread_exit(NULL);
}

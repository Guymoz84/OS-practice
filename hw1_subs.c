#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#define MAX_LENGTH_DIR 256
#define MAX_LENGTH_FILE 256
#define MAX_LENGTH_PATH 1024

int main(int argc,char** argv){
	struct stat for_size;
	int status;
	int size;
	int num_word_read=0;
	char* dir_folder;
	char* dst_file;
	char* tmp_buffer;
	char* read_from_file;
	if(argc!=3){
		printf("Number of arguments isn't as expected(not 3)!\n");
		return 1;
	}
	dir_folder=getenv("HW1DIR");
	if(dir_folder==NULL){
		printf("HW1DIR isn't correct!\n");
		return 1;
	}
	dst_file=getenv("HW1TF");
	if(dst_file==NULL){
		printf("HW1TF isn't correct!\n");
		return 1;
	}
    char* path=(char*)malloc(sizeof(char)*(strlen(dir_folder)+strlen(dst_file)+3));
	if(path==NULL){
		printf("Can't malloc for path\n");
		return 1;
	}
	path[0]='\0';
	strcat(path,dir_folder);
	strcat(path,"/");
	strcat(path,dst_file);
	int file_check=open(path,O_RDONLY);
	if(file_check<0){
		printf("Can't open the file!\n");
		free(path);
		return 1;
	}
	status=stat(path,&(for_size));
	if(status==-1){
		printf("The stat operation failed!\n");
		if(close(file_check)<0){
			printf("Can't close the file!\n");
		}
		free(path);
		return 1;
	}
	size=for_size.st_size;
	if(size<0){
		printf("File size isn't correct\n");
		close(file_check);
		free(path);
		return 1;
	}
	char* buffer=malloc(sizeof(char)*size+2);//1 is for the '\0'
	if(buffer==NULL){
		printf("Can't malloc for buffer\n");
		close(file_check);
		free(path);
		return 1;
	}
	tmp_buffer=buffer;
	while((num_word_read=read(file_check,buffer,size))!=0){
		if(num_word_read<0){
			printf("Can't read the file!\n");
			close(file_check);
			buffer=tmp_buffer;
			free(buffer);
			free(path);
			return 1;
		}
		size=size-num_word_read;
		buffer=(buffer+num_word_read);
	}
	buffer[0]='\0';
	buffer=tmp_buffer;
	while((read_from_file=strstr(buffer,argv[1]))!=NULL){
		num_word_read=read_from_file-buffer;
	    printf("%.*s",num_word_read,buffer);
	    printf("%s",argv[2]);
	    buffer=read_from_file+strlen(argv[1]);
	}
	printf("%s",buffer);
	buffer=tmp_buffer;
	free(buffer);
	free(path);
	close(file_check);
	if(file_check<0){
		printf("Can't close the file after finishing handling the file!\n");
		return 1;
	}
	return 0;
}

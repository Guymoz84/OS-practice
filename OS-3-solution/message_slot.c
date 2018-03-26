#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically,a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <asm/uaccess.h>    /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");

#include "message_slot.h"

typedef struct channel_node{
	unsigned long channel_num;
	char message[BUF_LEN];
	int msg_size;
	struct channel_node* next;
}Channel;

typedef struct file_node{
	unsigned long current_channel;
	unsigned long minorNumber;
	Channel* channels;
	struct file_node* next;
}File_node;

struct Main_dev{
	File_node* main_file;
};

static void destroySlots(File_node* file_node);
static void destroyChannels(Channel* channel);
static File_node* findNode(File_node* file_node,unsigned int minorNumber);
static Channel* findChannel(Channel* channel,unsigned int channel_num);

static struct Main_dev main_dev;

static int device_open(struct inode* inode,struct file* file){
	//We don't want to talk to two processes at the same time
	File_node* file_node;
	unsigned long minor_num;
	//the code goes here
	if(inode==NULL||file==NULL){
		return -EINVAL;//sets errno to EINVAL and returns -1
	}
	minor_num=iminor(file_inode(file));
	file_node=findNode(main_dev.main_file,minor_num);
	if(file_node==NULL){//need to create a new file_node
		file_node=(File_node*)kmalloc(sizeof(File_node),GFP_KERNEL);
		if(file_node==NULL){
			return -ENOMEM;//sets errno to ENOMEM and returns -1
		}
		file_node->minorNumber=minor_num;
		file_node->current_channel=0;
		file_node->channels=NULL;
		file_node->next=NULL;
		file_node->next=main_dev.main_file;
		main_dev.main_file=file_node;
	}
	return SUCCESS;
}
static int device_release( struct inode* inode,struct file* file){
	//nothing to do
	return SUCCESS;
}
//a process which has already opened
//the device file attempts to read from it
static ssize_t device_read(struct file* file,char __user* buffer,size_t length,loff_t* offset){
	File_node* file_node;
	Channel* channel;
	char* msg_ptr;
	unsigned long minor_num;
	unsigned long channel_num;
	int length_msg=0;
	int bytes_read=0;
	int test_put_user=0;
	if(file==NULL||buffer==NULL){
		return -EFAULT;//sets errno to EFAULT and returns -1
	}
	minor_num=iminor(file_inode(file));
	file_node=findNode(main_dev.main_file,minor_num);
	if(file_node==NULL){
		return -EINVAL;//sets errno to EINVAL and returns -1
	}
	//found the file_node,need to find the channel
	channel_num=file_node->current_channel;
	channel=findChannel(file_node->channels,channel_num);
	if(channel==NULL){
		return -EINVAL;//sets errno to EINVAL and returns -1
	}
	msg_ptr=channel->message;
	length_msg=channel->msg_size;//instead of strlen - maybe the mesaage has a null char(not at the end)
	if(length_msg==0){
		return -EWOULDBLOCK;//sets errno to EWOULDBLOCK and returns -1
	}
	if(length_msg>=length){
		return -ENOSPC;//sets errno to ENOSPC and returns -1
	}
	while(length_msg>bytes_read&&BUF_LEN>bytes_read){
		test_put_user=put_user(msg_ptr[bytes_read],&buffer[bytes_read]);
		if(test_put_user!=0){
			return -EFAULT;//sets errno to EFAULT and returns -1
		}
		bytes_read++;
	}
	//return the number of input characters written
	return bytes_read;
}
//a processs which has already opened
//the device file attempts to write to it
static ssize_t device_write(struct file* file,const char __user* buffer,size_t length,loff_t* offset){
	File_node* file_node;
	Channel* channel;
	char* msg_ptr;
	char tmp_buf[BUF_LEN];//if get_user fails - keep the prev message
	unsigned long minor_num;
	unsigned long channel_num;
	int bytes_write=0;
	int test_get_user;
	if(buffer==NULL){
		return -EFAULT;//sets errno to EFAULT and returns -1
	}
	minor_num=iminor(file_inode(file));
	file_node=findNode(main_dev.main_file,minor_num);
	if(file_node==NULL){//the file_node wasn't found
		return -EINVAL;//sets errno to EINVAL and returns -1
	}
	//found the file_node,need to find the channel
	channel_num=file_node->current_channel;
	channel=findChannel(file_node->channels,channel_num);
	if(channel==NULL){
		return -EINVAL;//sets errno to EINVAL and returns -1
	}
	msg_ptr=channel->message;
	if(length>BUF_LEN||length<=0){
		return -EINVAL;//sets errno to EINVAL and returns -1
	}
	while(length>bytes_write&&BUF_LEN>bytes_write){
		test_get_user=get_user(tmp_buf[bytes_write],&buffer[bytes_write]);
		if(test_get_user!=0){
			return -EFAULT;
		}
		bytes_write++;
	}
	channel->msg_size=bytes_write;
	memcpy(msg_ptr,tmp_buf,bytes_write);//only if get_user is fine, copy the message
	//return the number of input characters written
	return bytes_write;
}
//----------------------------------------------------------------
static long device_ioctl( struct   file* file,unsigned int ioctl_command_id,unsigned long  ioctl_param){
	File_node* file_node;
	unsigned long minor_num;
	unsigned long channel_num;
	Channel* channel;
	if(file==NULL){
		return -EINVAL;//sets errno to EINVAL and returns -1
	}
	//Switch according to the ioctl called
	if(ioctl_command_id==MSG_SLOT_CHANNEL){
		minor_num=iminor(file_inode(file));
		file_node=findNode(main_dev.main_file,minor_num);
		if(file_node==NULL){//the file_node wasn't found
			return -EINVAL;//sets errno to EINVAL and returns -1
		}
		//found the file_node,need to find the channel
		file_node->current_channel=ioctl_param;
		channel_num=file_node->current_channel;
		channel=findChannel(file_node->channels,channel_num);
		if(channel==NULL){//the channel wasn't found,need to create a new one
			channel=(Channel*)kmalloc(sizeof(Channel),GFP_KERNEL);
			if(channel==NULL){
				return -ENOMEM;//sets errno to ENOMEM and returns -1
			}
			channel->channel_num=channel_num;
			channel->msg_size=0;
			channel->message[0]='\0';
			channel->next=NULL;
			channel->next=file_node->channels;
			file_node->channels=channel;
		}
		//Get the parameter given to ioctl by the process
		return SUCCESS;
	}
	else{
		//If the passed command is not MSG_SLOT_CHANNEL,returns -1 and sets errno to EINVAL.
		return -EINVAL;//sets errno to EINVAL and returns -1
	}
}
//This structure will hold the functions to be called
//when a process does something to the device we created
struct file_operations Fops=
{
  .open         =device_open,
  .read         =device_read,
  .release      =device_release,
  .write        =device_write,
  .unlocked_ioctl=device_ioctl,

};
static int __init message_slot_init(void){
	int check=-1;
	memset(&main_dev,0,sizeof(struct Main_dev));
	//Register driver capabilities. Obtain major num
	check=register_chrdev(majorNumber,DEVICE_RANGE_NAME,&Fops);
	if(check<0){
		return -1;
	}
	printk(KERN_INFO "message_slot: registered major number %d\n",majorNumber);
	main_dev.main_file=NULL;
	return SUCCESS;
}
//--- unloader -------------------------------------------
static void __exit message_slot_cleanup(void){
	destroySlots(main_dev.main_file);
	unregister_chrdev(majorNumber,DEVICE_RANGE_NAME);
}

//--------------------------------------------------------
module_init(message_slot_init);
module_exit(message_slot_cleanup);
static void destroySlots(File_node* file_node){
	if(file_node!=NULL){
		destroySlots(file_node->next);
		destroyChannels(file_node->channels);
		kfree(file_node);
	}
}
static void destroyChannels(Channel* channel){
	if(channel!=NULL){
		destroyChannels(channel->next);
		kfree(channel);
	}
}
static File_node* findNode(File_node* file_node,unsigned int minorNumber){
	while(file_node!=NULL){
		if(file_node->minorNumber==minorNumber){
			return file_node;
		}
		file_node=file_node->next;
	}
	return NULL;
}
static Channel* findChannel(Channel* channel,unsigned int channel_num){
	while(channel!=NULL){
		if(channel->channel_num==channel_num){
			return channel;
		}
		channel=channel->next;
	}
	return NULL;
}

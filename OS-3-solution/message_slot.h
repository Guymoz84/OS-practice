#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

// The major device number
#define majorNumber 244

// Set the message of the device driver
#define  MSG_SLOT_CHANNEL _IOW(majorNumber, 0, unsigned long)

#define DEVICE_RANGE_NAME "char_dev"
#define BUF_LEN 128
#define SUCCESS 0

#endif

#ifndef MESSAGES_H_
#define MESSAGE_H_

#include <string.h>

enum MessageType{
	HELLO_REC,
	HELLO_ANS,
	ACK_OK,
	ACK_PLAY,
	ERROR_1,
	ERROR_2,
	ERROR_3,
	ERROR_4,
	UNKNOWN
};


enum MessageType GetMessageType(char * msg);
void GetErrorMessage(enum MessageType msgType, char * trgt );


#endif

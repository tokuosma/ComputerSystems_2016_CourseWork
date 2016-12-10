
#include "Helpers/messages.h"

enum MessageType GetMessageType(char * msg){
	if(strcmp(msg, "Terve") == 0){
		return HELLO_REC;
	}
	else if(strcmp(msg, "Terve:") == 0){
		return HELLO_ANS;
	}
	else{
		return UNKNOWN;
	}
}

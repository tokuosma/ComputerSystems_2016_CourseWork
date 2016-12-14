
#include "Helpers/messages.h"


const char ERROR_1_MSG[16] = "1:UNKNOWN CMD";
const char ERROR_2_MSG[16] = "2:MALFORMED CMD";
const char ERROR_3_MSG[16] = "3:DONKEY EXISTS";
const char ERROR_4_MSG[16] = "4:DNKY NOT FOUND";
const char ERROR_UNKNOWN_MSG[16] = "UNKNOWN MSG";
const char ERROR_SERVER_TIMEOUT[16] = "SERVER TIMEOUT";

enum MessageType GetMessageType(char * msg){
	if(strcmp(msg, "Terve\n") == 0){
		return HELLO_REC;
	}
	else if(strncmp("OK",msg , 2) == 0){
		if(strlen(msg) == 3){
		  return ACK_OK;
		}
		else if(strlen(msg) > 3){
		  return ACK_PLAY;
		}
	}
	else if(strncmp(msg, "Terve:", 6) == 0){
		return HELLO_ANS;
	}
	else if(strncmp(msg, "Virhe:1", 7) == 0){
		return ERROR_1;
	}
	else if(strncmp(msg, "Virhe:2", 7) == 0){
		return ERROR_2;
	}
	else if(strncmp(msg, "Virhe:3", 7) == 0){
		return ERROR_3;
	}
	else if(strncmp(msg, "Virhe:4", 7) == 0){
		return ERROR_4;
	}
	else{
		return UNKNOWN;
	}
}


void GetErrorMessage(enum MessageType msgType, char * trgt ){
	switch(msgType){
		case ERROR_1:
			strcpy(trgt, ERROR_1_MSG);
			break;
		case ERROR_2:
			strcpy(trgt, ERROR_2_MSG);
			break;
		case ERROR_3:
			strcpy(trgt, ERROR_3_MSG);
			break;
		case ERROR_4:
			strcpy(trgt, ERROR_4_MSG);
			break;
		case UNKNOWN:
			strcpy(trgt, ERROR_UNKNOWN_MSG);
			break;
		default:
			break;
	}
}

/*
*Used Code:
* mpu_bmp_example.c
* Author: Teemu Leppanen / UBIComp / University of Oulu
*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
/* XDCtools files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/i2c/I2CCC26XX.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/PWM.h>
#include <ti/mw/display/Display.h>
#include <ti/mw/display/DisplayExt.h>
#include <ti/mw/remotecontrol/buzzer.h>


/* Board Header files */
#include "Board.h"

#include "wireless/comm_lib.h"
#include "Helpers/messages.h"
#include "Helpers/magnify.h"
#include "sensors/bmp280.h"
#include "sensors/opt3001.h"
#include "sensors/tmp007.h"
#include "sensors/mpu9250.h"
#include "Aasi.h"


// *******************************
//
// MPU GLOBAL VARIABLES
//
// *******************************
static PIN_Handle hMpuPin;
static PIN_State MpuPinState;
static PIN_Config MpuPinConfig[] = {
    Board_MPU_POWER  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

// MPU9250 uses its own I2C interface
static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1
};

/* AASI */

struct Aasi aasi = {
	.Name = "Duffy",
	.Move = 0,
	.Sun = 0,
	.Air = 0,
	.Social = 0,
	.Image[0] = 0x81,
	.Image[1] = 0x42,
	.Image[2] = 0x7E,
	.Image[3] = 0xA5,
	.Image[4] = 0x81,
	.Image[5] = 0x5A,
	.Image[6] = 0x24,
	.Image[7] = 0x18,
	.Active = false
};

const char AASI_NAME[16] = "Duffy";


struct Aasi NEW_AASI = {
	.Name = "Duffy",
	.Move = 0,
	.Sun = 0,
	.Air = 0,
	.Social = 0,
	.Image[0] = 0x81,
	.Image[1] = 0x42,
	.Image[2] = 0x7E,
	.Image[3] = 0xA5,
	.Image[4] = 0x81,
	.Image[5] = 0x5A,
	.Image[6] = 0x24,
	.Image[7] = 0x18,
	.Active = false
};


// ICONS
const uint8_t IconMove[8] = {
		0x00,0xE0,0xE0,0xE0,0xE0,0xFE,0xFF,0xFF
};
const uint8_t IconSun[8] = {
		0x89,0x4A,0x3C,0xFC,0x3F,0x3C,0x52,0x91
};
const uint8_t IconAir[8] = {
		0x10,0x38,0x7C,0xFE,0x38,0x10,0x10,0x10
};
const uint8_t IconSocial[8] = {
		0x3C,0x42,0xA5,0x81,0xA5,0x99,0x42,0x3C
};
const uint8_t IconArrow[8] = {
		0x00,0x08,0x0C,0xFE,0xFF,0xFE,0x0C,0x08
};

// GLOBAL SENSOR LIMITS
const double MOVE_LIMIT = 0;
const double TMP_LIMIT = 0;
const double LIGHT_LIMIT = 0;
const double PRES_LIMIT = 0;

/* Task */
#define STACKSIZE 3500
Char taskStack[STACKSIZE];
Char taskCommStack[STACKSIZE];
Char taskSensorsStack[STACKSIZE];

/* Display */
Display_Handle hDisplay;

enum DisplayStates {
   MAIN_0,
   MAIN_1,
   MENU_0_0,
   MENU_0_1,
   MENU_0_2,
   MENU_1_0,
   MENU_1_1,
   MENU_1_2,
   WAIT_REPLY_NEW,
   WAIT_REPLY_PLAY,
   WAIT_REPLY_SLEEP,
   SUCCESS,
   ERROR
};

const uint32_t imgPalette[] = {0, 0xFFFFFF};
tImage aasiImage ;
// Storage for the magnified aasi image
uint8_t aasiImageMag[128] = {0};
// Global display state
enum DisplayStates DisplayState = MAIN_0;
bool DisplayChanged = true;

// Icons
const tImage moveImage = {
		.BPP = IMAGE_FMT_1BPP_UNCOMP,
		.NumColors = 2,
		.XSize = 1,
		.YSize = 8,
		.pPalette = imgPalette,
		.pPixel = IconMove
};

const tImage sunImage = {
		.BPP = IMAGE_FMT_1BPP_UNCOMP,
		.NumColors = 2,
		.XSize = 1,
		.YSize = 8,
		.pPalette = imgPalette,
		.pPixel = IconSun
};

const tImage airImage = {
		.BPP = IMAGE_FMT_1BPP_UNCOMP,
		.NumColors = 2,
		.XSize = 1,
		.YSize = 8,
		.pPalette = imgPalette,
		.pPixel = IconAir
};

const tImage socialImage = {
		.BPP = IMAGE_FMT_1BPP_UNCOMP,
		.NumColors = 2,
		.XSize = 1,
		.YSize = 8,
		.pPalette = imgPalette,
		.pPixel = IconSocial
};

const tImage arrowImage = {
		.BPP = IMAGE_FMT_1BPP_UNCOMP,
		.NumColors = 2,
		.XSize = 1,
		.YSize = 8,
		.pPalette = imgPalette,
		.pPixel = IconArrow
};

/* Communication */
// Communication error message
char ERROR_MSG[16] = "";
uint8_t receiveStatus;

// Boolean to determine if a request was sent to server
bool AwaitingReplyNew = false;
bool AwaitingReplyPlay = false;
bool AwaitingReplySleep = false;

/* Pin Button1 configured as power button */
static PIN_Handle hActionButton;
static PIN_State sPowerButton;
PIN_Config cPowerButton[] = {
    Board_BUTTON1 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    PIN_TERMINATE
};
PIN_Config cPowerWake[] = {
    Board_BUTTON1 | PIN_INPUT_EN | PIN_PULLUP | PINCC26XX_WAKEUP_NEGEDGE,
    PIN_TERMINATE
};

/* Pin Button0 configured */
static PIN_Handle hButton0;
static PIN_State sButton0;
PIN_Config cButton0[] = {
    Board_BUTTON0 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    PIN_TERMINATE
};

/* Leds */
static PIN_Handle hLed;
static PIN_State sLed;
PIN_Config cLed[] = {
    Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    Board_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

/* Clocks */
Clock_Handle serverTimeoutHandle;
Clock_Params serverTimeoutParams;
UInt serverTimeoutValue;


// BUTTON PROTOTYPES
Void button0_MAIN_0_FXN(PIN_Handle handle, PIN_Id pinId);
Void button0_MAIN_1_FXN(PIN_Handle handle, PIN_Id pinId);
Void button0_MENU_0_0_FXN(PIN_Handle handle, PIN_Id pinId);
Void button0_MENU_0_1_FXN(PIN_Handle handle, PIN_Id pinId);
Void button0_MENU_0_2_FXN(PIN_Handle handle, PIN_Id pinId);
Void button0_MENU_1_0_FXN(PIN_Handle handle, PIN_Id pinId);
Void button0_MENU_1_1_FXN(PIN_Handle handle, PIN_Id pinId);
Void button0_MENU_1_2_FXN(PIN_Handle handle, PIN_Id pinId);
Void button0_WAIT_REPLY_NEW_FXN(PIN_Handle handle, PIN_Id pinId);
Void button0_WAIT_REPLY_PLAY_FXN(PIN_Handle handle, PIN_Id pinId);
Void button0_ERROR_FXN(PIN_Handle handle, PIN_Id pinId);
Void actionButton_MAIN_FXN(PIN_Handle handle, PIN_Id pinId);
Void actionButton_MENU_0_0_FXN(PIN_Handle handle, PIN_Id pinId);
Void actionButton_MENU_0_1_FXN(PIN_Handle handle, PIN_Id pinId);
Void actionButton_MENU_0_2_FXN(PIN_Handle handle, PIN_Id pinId);
Void actionButton_MENU_1_0_FXN(PIN_Handle handle, PIN_Id pinId);
Void actionButton_MENU_1_1_FXN(PIN_Handle handle, PIN_Id pinId);
Void actionButton_MENU_1_2_FXN(PIN_Handle handle, PIN_Id pinId);
Void actionButton_ERROR_FXN(PIN_Handle handle, PIN_Id pinId);
//Void actionButton_WAIT_REPLY_NEW_FXN(PIN_Handle handle, PIN_Id pinId);
//Void actionButton_WAIT_REPLY_PLAY_FXN(PIN_Handle handle, PIN_Id pinId);
Void actionButton_IDLE_FXN(PIN_Handle handle, PIN_Id pinId);

// Helpers
Void SensorsRefreshDisplay();
Void CancelRequests();
Void SetDisplayState(enum DisplayStates newDisplayState);


Void serverTimeoutFxn(UArg arg0) {
	// Cancel requests
	CancelRequests();
	memcpy(ERROR_MSG, "SERVER TIMEOUT", 15);
	SetDisplayState(ERROR);
    Clock_delete(&serverTimeoutHandle);
}



/*STATE: Waiting for server reply after sending new Donkey*/
/*DO: Cancel request*/
Void button0_WAIT_REPLY_NEW_FXN(PIN_Handle handle, PIN_Id pinId){

	if(serverTimeoutHandle != NULL){
		Clock_stop(serverTimeoutHandle);
		Clock_delete(&serverTimeoutHandle);
	}
	CancelRequests();
	SetDisplayState(MAIN_0);

}

/*STATE: Waiting for server reply after sending play message*/
/*DO: Cancel request*/
Void button0_WAIT_REPLY_PLAY_FXN(PIN_Handle handle, PIN_Id pinId){

	if(serverTimeoutHandle != NULL){
		Clock_stop(serverTimeoutHandle);
		Clock_delete(&serverTimeoutHandle);
	}
	CancelRequests();
	SetDisplayState(MAIN_0);
}

/*STATE: Waiting for server reply after sending sleep message*/
/*DO: Cancel request*/
Void button0_WAIT_REPLY_SLEEP_FXN(PIN_Handle handle, PIN_Id pinId){

	if(serverTimeoutHandle != NULL){
		Clock_stop(serverTimeoutHandle);
		Clock_delete(&serverTimeoutHandle);
	}
	CancelRequests();
	SetDisplayState(MAIN_1);
}

/*STATE: Main view without donkey*/
/*DO: Open menu*/
Void button0_MAIN_0_FXN(PIN_Handle handle, PIN_Id pinId) {SetDisplayState(MENU_0_0);}

/*STATE: Main view with donkey*/
/*DO: Open menu*/
Void button0_MAIN_1_FXN(PIN_Handle handle, PIN_Id pinId) {SetDisplayState(MENU_1_0);}

/*STATE: Menu without donkey, first option (UUSI) selected*/
/*DO: Move to second menu option*/
Void button0_MENU_0_0_FXN(PIN_Handle handle, PIN_Id pinId) {SetDisplayState(MENU_0_1);}

/*STATE: Menu without donkey, second option (LEIKI) selected*/
/*DO: Move to third menu option */
Void button0_MENU_0_1_FXN(PIN_Handle handle, PIN_Id pinId) {SetDisplayState(MENU_0_2);}

/*STATE: Menu without donkey, third option (TAKAISIN) selected*/
/*DO: MOVE TO FIRST MENU OPTION*/
Void button0_MENU_0_2_FXN(PIN_Handle handle, PIN_Id pinId) {SetDisplayState(MENU_0_0);}

/*STATE: Menu with donkey, first option (NUKU) selected*/
/*DO: MOVE TO SECOND MENU OPTION*/
Void button0_MENU_1_0_FXN(PIN_Handle handle, PIN_Id pinId) {SetDisplayState(MENU_1_1);}

/*STATE: Menu with donkey, second option (MOIKKAA) selected*/
/*DO: MOVE TO THIRD MENU OPTION*/
Void button0_MENU_1_1_FXN(PIN_Handle handle, PIN_Id pinId) {SetDisplayState(MENU_1_2);}

/*STATE: Menu with donkey, third option (TAKAISIN) selected*/
/*DO: MOVE TO FIRST MENU OPTION*/
Void button0_MENU_1_2_FXN(PIN_Handle handle, PIN_Id pinId) {SetDisplayState(MENU_1_0);}

/*STATE: Communication error*/
/*DO: MOVE BACK TO MAIN VIEW*/
Void button0_ERROR_FXN(PIN_Handle handle, PIN_Id pinId) {
	if(aasi.Active == true){SetDisplayState(MAIN_1);}
	else{SetDisplayState(MAIN_0);}
}

/*STATE: In the main view*/
/*DO: Handle power button */
Void actionButton_MAIN_FXN(PIN_Handle handle, PIN_Id pinId) {
    Display_clear(hDisplay);
    Display_close(hDisplay);
    Task_sleep(100000 / Clock_tickPeriod);

	PIN_close(hActionButton);

    PINCC26XX_setWakeup(cPowerWake);
	Power_shutdown(NULL,0);
}

/*STATE: Menu without donkey, first option (UUSI) selected*/
/*DO: SEND NEW DONKEY TO SERVER*/
Void actionButton_MENU_0_0_FXN(PIN_Handle handle, PIN_Id pinId) {

	char payload[80] = "";

	serialize_aasi_new(NEW_AASI, payload );

	Send6LoWPAN(IEEE80154_SINK_ADDR, payload, strlen(payload));
	AwaitingReplyNew = true;
	receiveStatus = StartReceive6LoWPAN();

	SetDisplayState(WAIT_REPLY_NEW);

    serverTimeoutHandle = Clock_create((Clock_FuncPtr) serverTimeoutFxn, serverTimeoutValue, &serverTimeoutParams, NULL);
    if (serverTimeoutHandle == NULL) {
    	System_abort("Clock create failed");
    }
    Clock_start(serverTimeoutHandle);
}

/*STATE: Menu without donkey, second option (LEIKI) selected*/
/*DO: CALL DONKEY FROM SERVER*/
Void actionButton_MENU_0_1_FXN(PIN_Handle handle, PIN_Id pinId) {

	char payload[80] = "";
	serialize_aasi_play(payload, AASI_NAME);


	Send6LoWPAN(IEEE80154_SINK_ADDR, payload, strlen(payload));
	AwaitingReplyPlay = true;
	receiveStatus = StartReceive6LoWPAN();

	SetDisplayState(WAIT_REPLY_PLAY);

	serverTimeoutHandle = Clock_create((Clock_FuncPtr)serverTimeoutFxn, serverTimeoutValue, &serverTimeoutParams, NULL);
	if(serverTimeoutHandle == NULL){
    	System_abort("Clock create failed");
	}
	Clock_start(serverTimeoutHandle);
}

/*STATE: Menu without donkey, third option (TAKAISIN) selected*/
/*DO: MOVE BACK TO MAIN VIEW*/
Void actionButton_MENU_0_2_FXN(PIN_Handle handle, PIN_Id pinId) {SetDisplayState(MAIN_0);}

/*STATE: Menu with donkey, first option (NUKU) selected*/
/*DO: PUT DONKEY TO SLEEP, SEND STATS TO SERVER*/
Void actionButton_MENU_1_0_FXN(PIN_Handle handle, PIN_Id pinId) {

	char payload[80] = "";
	serialize_aasi_sleep(aasi, payload);

	Send6LoWPAN(IEEE80154_SINK_ADDR, payload, strlen(payload));
	AwaitingReplySleep = true;
	receiveStatus = StartReceive6LoWPAN();

	SetDisplayState(WAIT_REPLY_SLEEP);

	serverTimeoutHandle = Clock_create((Clock_FuncPtr)serverTimeoutFxn, serverTimeoutValue, &serverTimeoutParams, NULL);
	if(serverTimeoutHandle == NULL){
		System_abort("Clock create failed");
	}
	Clock_start(serverTimeoutHandle);
}

/*STATE: Menu with donkey, second option (MOIKKAA) selected*/
/*DO: SEND BROADCAST MESSAGE*/
Void actionButton_MENU_1_1_FXN(PIN_Handle handle, PIN_Id pinId) {

    // Test if previous message send complete
	Send6LoWPAN(IEEE80154_BROADCAST_ADDR, "Terve\n", 6);
	StartReceive6LoWPAN();

	SetDisplayState(MAIN_1);
}

/*STATE: Menu with donkey, third option (TAKAISIN) selected*/
/*DO: MOVE BACK TO MAIN VIEW*/
Void actionButton_MENU_1_2_FXN(PIN_Handle handle, PIN_Id pinId) {SetDisplayState(MAIN_1);}

/*STATE: Communication error*/
/*DO: NOTHING*/
Void actionButton_ERROR_FXN(PIN_Handle handle, PIN_Id pinId) {}


/*STATE: Waiting for reply*/
/*DO: NOTHING*/
Void actionButton_IDLE_FXN(PIN_Handle handle, PIN_Id pinId){}

/* Communication Task */
Void commFxn(UArg arg0, UArg arg1) {

	uint16_t senderAddr;
	char buffer[80] = "";
	char empty[80] = "";
	enum MessageType msgType;
    // Radio to receive mode
	int32_t result = StartReceive6LoWPAN();
	if(result != true) {
		System_abort("Wireless receive mode failed");
	}

	// TASK LOOP
    while (1) {
    	// MESSAGE RECEIVED
    	if (GetRXFlag() == true) {
    		// RECEIVE MESSAGE, SAVE SENDER ADDRESS
			Receive6LoWPAN(&senderAddr, buffer, 80);
			msgType = GetMessageType(buffer);
			// SKIP BAD MESSAGES;
			if(msgType == UNKNOWN){
				memcpy(buffer, empty, 80); // Clear buffer
				continue;
			}
			// Received greeting -> Send reply, update social
			else if(msgType == HELLO_REC){
				if(aasi.Active == true){
					sprintf(buffer, "Terve:%s\n", aasi.Name );
					Send6LoWPAN(senderAddr, buffer, strlen(buffer));
					StartReceive6LoWPAN();
					aasi.Social = aasi.Social + 1;
					SensorsRefreshDisplay();
				}
			}
			// Received reply -> Update social
			else if(msgType == HELLO_ANS){
				if(aasi.Active == true){
					aasi.Social = aasi.Social + 1;
					SensorsRefreshDisplay();
				}

			}
			// Awaiting reply, receive error msg --> Move to error view
			else if((msgType == ERROR_1 || msgType == ERROR_2 || msgType == ERROR_3 ||	msgType == ERROR_4 ))
			{
				if(AwaitingReplyNew == true || AwaitingReplyPlay == true || AwaitingReplySleep == true){
					if(serverTimeoutHandle != NULL){
						Clock_stop(serverTimeoutHandle);
						Clock_delete(&serverTimeoutHandle);
					}
					CancelRequests();
					GetErrorMessage(msgType, ERROR_MSG);
					SetDisplayState(ERROR);
				}
			}
			// RECEIVE OK FROM SERVER
			else if(msgType == ACK_OK){
				// NEW DONKEY CREATED SUCCESFULLY
				if(AwaitingReplyNew == true){
					if(serverTimeoutHandle != NULL){
						Clock_stop(serverTimeoutHandle);
						Clock_delete(&serverTimeoutHandle);
					}
					CancelRequests();
					SetDisplayState(MAIN_0);
				}
				// PUT DONKEY TO SLEEP
				else if(AwaitingReplySleep == true){
					if(serverTimeoutHandle != NULL){
						Clock_stop(serverTimeoutHandle);
						Clock_delete(&serverTimeoutHandle);
					}
					CancelRequests();
					aasi.Active = false;
					SetDisplayState(MAIN_0);
				}
				else
				{
					CancelRequests();
					memcpy(buffer, empty, 80); // Clear buffer
					continue;
				}
			}

			// RECEIVED NEW PLAY MATE FROM SERVER
			else if((msgType == ACK_PLAY))
			{
				if(AwaitingReplyPlay){
					if(serverTimeoutHandle != NULL){
						Clock_stop(serverTimeoutHandle);
						Clock_delete(&serverTimeoutHandle);
					}

					CancelRequests();
					struct Aasi newAasi = deserialize_aasi_play(buffer);
					aasi = newAasi;
					magnify(aasi.Image, aasiImageMag);
					tImage tempImage =  {
						.BPP = IMAGE_FMT_1BPP_UNCOMP,
						.NumColors = 2,
						.XSize = 31,
						.YSize = 32,
						.pPalette = imgPalette,
						.pPixel = aasiImageMag
					};
					aasiImage = tempImage;
					SetDisplayState(MAIN_1);
				}
			}
			memcpy(buffer, empty, 80); // Clear buffer
		} // ENDIF
    } //END WHILE
}

Void displayFxn(UArg arg0, UArg arg1) {

	// Initialize display variables

	magnify(aasi.Image, aasiImageMag);
	const tImage initialImage =  {
		.BPP = IMAGE_FMT_1BPP_UNCOMP,
		.NumColors = 2,
		.XSize = 31,
		.YSize = 32,
		.pPalette = imgPalette,
		.pPixel = aasiImageMag
	};

	aasiImage = initialImage;

    Display_Params displayParams;
	displayParams.lineClearMode = DISPLAY_CLEAR_BOTH;
    Display_Params_init(&displayParams);

    hDisplay = Display_open(Display_Type_LCD, &displayParams);
    if (hDisplay == NULL) {
        System_abort("Error initializing Display\n");
    }
    tContext *pContext = DisplayExt_getGrlibContext(hDisplay);
    if (pContext == NULL) {
    	System_abort("Error initializing graphics library\n");
    }

    char stats_line_1[16];
    char stats_line_2[16];
    while (1) {
    	if(DisplayChanged == true){
			Display_clear(hDisplay);

			if(DisplayState == MAIN_1){

				sprintf(stats_line_1, "   %4d   %4d", aasi.Move, aasi.Sun);
				sprintf(stats_line_2, "   %4d   %4d", aasi.Air, aasi.Social);
				Display_print0(hDisplay, 10, 5, aasi.Name);

				Display_print0(hDisplay, 1, 0, stats_line_1);
				Display_print0(hDisplay, 2, 0, stats_line_2);

				GrImageDraw(pContext, &aasiImage, 32, 40);
				GrImageDraw(pContext, &moveImage, 12, 7);
				GrImageDraw(pContext, &sunImage, 54, 7);
				GrImageDraw(pContext, &airImage, 12, 15);
				GrImageDraw(pContext, &socialImage, 54, 15);
				GrLineDraw(pContext,0,24,96,24);
				GrFlush(pContext);

			}
			else if(DisplayState == MAIN_0){
				Display_print0(hDisplay, 10, 4, "Ei aasia.");
			}
			else if(DisplayState == MENU_0_0){
				Display_print0(hDisplay, 1, 6, "MENU");

				Display_print0(hDisplay, 4, 4, "UUSI");
				Display_print0(hDisplay, 5, 4, "LEIKI");
				Display_print0(hDisplay, 6, 4, "TAKAISIN");

				GrImageDraw(pContext, &arrowImage, 14, 30);
				GrFlush(pContext);
			}
			else if(DisplayState == MENU_0_1){
				Display_print0(hDisplay, 1, 6, "MENU");

				Display_print0(hDisplay, 4, 4, "UUSI");
				Display_print0(hDisplay, 5, 4, "LEIKI");
				Display_print0(hDisplay, 6, 4, "TAKAISIN");
				GrImageDraw(pContext, &arrowImage, 14, 38);
				GrFlush(pContext);

			}
			else if(DisplayState == MENU_0_2){
				Display_print0(hDisplay, 1, 6, "MENU");

				Display_print0(hDisplay, 4, 4, "UUSI");
				Display_print0(hDisplay, 5, 4, "LEIKI");
				Display_print0(hDisplay, 6, 4, "TAKAISIN");
				GrImageDraw(pContext, &arrowImage, 14, 46);
				GrFlush(pContext);
			}

			else if(DisplayState == MENU_1_0){
				Display_print0(hDisplay, 1, 6, "MENU");
				Display_print0(hDisplay, 4, 4, "NUKU");
				Display_print0(hDisplay, 5, 4, "MOIKKAA");
				Display_print0(hDisplay, 6, 4, "TAKAISIN");

				GrImageDraw(pContext, &arrowImage, 14, 30);
				GrFlush(pContext);

			}
			else if(DisplayState == MENU_1_1){
				Display_print0(hDisplay, 1, 6, "MENU");
				Display_print0(hDisplay, 4, 4, "NUKU");
				Display_print0(hDisplay, 5, 4, "MOIKKAA");
				Display_print0(hDisplay, 6, 4, "TAKAISIN");

				GrImageDraw(pContext, &arrowImage, 14, 38);
				GrFlush(pContext);

			}
			else if(DisplayState == MENU_1_2){
				Display_print0(hDisplay, 1, 6, "MENU");
				Display_print0(hDisplay, 4, 4, "NUKU");
				Display_print0(hDisplay, 5, 4, "MOIKKAA");
				Display_print0(hDisplay, 6, 4, "TAKAISIN");

				GrImageDraw(pContext, &arrowImage, 14, 46);
				GrFlush(pContext);

			}
			else if(DisplayState == WAIT_REPLY_NEW){
				Display_print0(hDisplay, 4, 4, "ODOTETAAN");
				Display_print0(hDisplay, 5, 4, "VASTAUSTA");
				Display_print0(hDisplay, 8, 1, "Paina nappia");
				Display_print0(hDisplay, 9, 1, "peruuttaaksesi");
			}
			else if(DisplayState == WAIT_REPLY_PLAY){
				Display_print0(hDisplay, 4, 4, "KUTSUTAAN");
				Display_print0(hDisplay, 5, 4, "AASIA");
				Display_print0(hDisplay, 8, 1, "Paina nappia");
				Display_print0(hDisplay, 9, 1, "peruuttaaksesi");
			}
			else if(DisplayState == WAIT_REPLY_SLEEP){
				Display_print0(hDisplay, 4, 4, "NUKUTETAAN");
				Display_print0(hDisplay, 5, 4, "AASIA");
				Display_print0(hDisplay, 8, 1, "Paina nappia");
				Display_print0(hDisplay, 9, 1, "peruuttaaksesi");
			}
			else if(DisplayState == ERROR){
				Display_print0(hDisplay, 1, 6, "VIRHE");

				Display_print0(hDisplay, 4, 0, ERROR_MSG);
			}
			else{
				Display_print0(hDisplay, 3, 3, "MOI");
			}
			DisplayChanged = false;
    	}
    	Task_sleep(1000000 / Clock_tickPeriod);
    }
}

Void sensorsFxn(UArg arg0, UArg arg1) {

	//Handles and parameters for I2C
	I2C_Handle i2c;
	I2C_Params i2cParams;
	I2C_Handle i2cMPU;
	I2C_Params i2cMPUParams;

	double accel;
	float ax, ay, az, gx, gy, gz;
    float temperature = 0;
    float light = 0;
    double pressure = 0;
    int sensor_count = 9;
	double temp_p;
	int move_count = 0;

     //Create I2C for usage
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;

	//Create second I2C for accelerometer
	I2C_Params_init(&i2cMPUParams);
	i2cMPUParams.bitRate = I2C_400kHz;
	i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;

	//Open I2CMPU, setup MPU9250 and close I2C
	i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
	if (i2cMPU == NULL) {
	 System_abort("Error Initializing I2CMPU\n");
	}
	PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_ON);
	Task_sleep(100000 / Clock_tickPeriod);
	System_printf("MPU9250: Power ON\n");
	System_flush();

	mpu9250_setup(&i2cMPU);
	I2C_close(i2cMPU);

	//Open I2C, setup other sensors, then close I2C
	i2c = I2C_open(Board_I2C0, &i2cParams);
	if (i2c == NULL) {
	 System_abort("Error Initializing I2C\n");
	}
	else {
	 System_printf("I2C Initialized!\n");
	}
	tmp007_setup(&i2c);
	opt3001_setup(&i2c);
	bmp280_setup(&i2c);
	I2C_close(i2c);


    //Infinite sensor reading loop
	while (1){
		//Check if donkey is in the machine
		if (aasi.Active == true) {
			//Sensors read data on every 10th cycle = once per second
			if (sensor_count == 9) {
				sensor_count = 0;
				i2c = I2C_open(Board_I2C, &i2cParams);
				temperature = tmp007_get_data(&i2c);
				light = opt3001_get_data(&i2c);
				if (light > 200 && temperature >= 15) {
					aasi.Sun = aasi.Sun + 1;
					DisplayChanged = true;
				}

				bmp280_get_data(&i2c, &pressure, &temp_p);
				if (pressure > 1020,9 && pressure < 1021,9) {
					aasi.Air = aasi.Air + 1;
					DisplayChanged = true;
				}
				I2C_close(i2c);
			}
			sensor_count = sensor_count + 1;
			i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
			if (i2cMPU == NULL) {
				System_abort("Error Initializing I2CMPU\n");
			}
			mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
			//Take RMS of acceleration data
			accel = sqrt(pow(ax,2) + pow(ay,2) + pow(az,2));
			//Acceleration sensor reads data on every cycle = once every 100ms,
			//Increases aasi.Move every 10th cycle
			if (accel > 1.5) {
				move_count = move_count + 1;
				if (move_count == 10) {
					aasi.Move = aasi.Move + 1;
					move_count = 0;
					DisplayChanged = true;
				}
				}
			I2C_close(i2cMPU);
		}
		//Delay on sensors task is 100ms
		Task_sleep(100000 / Clock_tickPeriod);
	}
	//Outside loop, program shouldn't get here anyway
	PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_OFF);
}

Int main(void) {

    // Task variables
	Task_Handle task;
	Task_Params taskParams;
	Task_Handle taskComm;
	Task_Params taskCommParams;
	Task_Handle taskSensors;
	Task_Params taskSensorsParams;

    // Initialize board
	Board_initGeneral();

	//Initialize i2c
	Board_initI2C();

    /* Clocks */
    serverTimeoutValue = 200000000 / Clock_tickPeriod;
    Clock_Params_init(&serverTimeoutParams);
    serverTimeoutParams.period = serverTimeoutValue;
    serverTimeoutParams.startFlag = FALSE;

	/* Buttons */
	hActionButton = PIN_open(&sPowerButton, cPowerButton);
	if(!hActionButton) {
		System_abort("Error initializing button shut pins\n");
	}
	if (PIN_registerIntCb(hActionButton, &actionButton_MAIN_FXN) != 0) {
		System_abort("Error registering button callback function");
	}

	hButton0 = PIN_open(&sButton0, cButton0);
		if(!hButton0) {
			System_abort("Error initializing button 0 pins\n");
		}
		if (PIN_registerIntCb(hButton0, &button0_MAIN_0_FXN) != 0) {
			System_abort("Error registering button callback function");
		}

    /* Leds */
    hLed = PIN_open(&sLed, cLed);
    if(!hLed) {
        System_abort("Error initializing LED pin\n");
    }

    hMpuPin = PIN_open(&MpuPinState, MpuPinConfig);
    if (hMpuPin == NULL) {
      	System_abort("Pin open failed!");
    }

    /* Display Task */
    Task_Params_init(&taskParams);
    taskParams.stackSize = STACKSIZE;
    taskParams.stack = &taskStack;
    taskParams.priority=2;

    task = Task_create(displayFxn, &taskParams, NULL);
    if (task == NULL) {
    	System_abort("Task create failed!");
    }

    /* Communication Task */
    Init6LoWPAN();

    Task_Params_init(&taskCommParams);
    taskCommParams.stackSize = STACKSIZE;
    taskCommParams.stack = &taskCommStack;
    taskCommParams.priority=1;

    taskComm = Task_create(commFxn, &taskCommParams, NULL);
    if (taskComm == NULL) {
    	System_abort("Comm task create failed!");
    }

    //Sensors Task
    Task_Params_init(&taskSensorsParams);
    taskSensorsParams.stackSize = STACKSIZE;
    taskSensorsParams.stack = &taskSensorsStack;
    taskSensorsParams.priority=2;

    taskSensors = Task_create(sensorsFxn, &taskSensorsParams, NULL);
    if (taskSensors == NULL) {
       	System_abort("Sensors task create failed!");
    }

    System_printf("Hello world!\n");
    System_flush();
    
    /* Start BIOS */
    BIOS_start();

    return (0);
}

// Set new display and change button interrupt callbacks
Void SetDisplayState(enum DisplayStates newDisplayState){

	DisplayChanged = true;
	switch(newDisplayState){
	case MAIN_0:
		DisplayState = MAIN_0;
		if (PIN_registerIntCb(hActionButton, &actionButton_MAIN_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		if (PIN_registerIntCb(hButton0, &button0_MAIN_0_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		break;

	case MAIN_1:
		DisplayState = MAIN_1;
		if (PIN_registerIntCb(hActionButton, &actionButton_MAIN_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		if (PIN_registerIntCb(hButton0, &button0_MAIN_1_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		break;

	case MENU_0_0:
		DisplayState = MENU_0_0;
		if (PIN_registerIntCb(hActionButton, &actionButton_MENU_0_0_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		if (PIN_registerIntCb(hButton0, &button0_MENU_0_0_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		break;
	case MENU_0_1:
		DisplayState = MENU_0_1;
		if (PIN_registerIntCb(hActionButton, &actionButton_MENU_0_1_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		if (PIN_registerIntCb(hButton0, &button0_MENU_0_1_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		break;

	case MENU_0_2:
		DisplayState = MENU_0_2;
		if (PIN_registerIntCb(hActionButton, &actionButton_MENU_0_2_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		if (PIN_registerIntCb(hButton0, &button0_MENU_0_2_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		break;

	case MENU_1_0:
		DisplayState = MENU_1_0;
		if (PIN_registerIntCb(hActionButton, &actionButton_MENU_1_0_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		if (PIN_registerIntCb(hButton0, &button0_MENU_1_0_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		break;
	case MENU_1_1:
		DisplayState = MENU_1_1;
		if (PIN_registerIntCb(hActionButton, &actionButton_MENU_1_1_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		if (PIN_registerIntCb(hButton0, &button0_MENU_1_1_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		break;

	case MENU_1_2:
		DisplayState = MENU_1_2;
		if (PIN_registerIntCb(hActionButton, &actionButton_MENU_1_2_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		if (PIN_registerIntCb(hButton0, &button0_MENU_1_2_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		break;

	case WAIT_REPLY_NEW:
		DisplayState = WAIT_REPLY_NEW;
		if (PIN_registerIntCb(hActionButton, &actionButton_IDLE_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		if (PIN_registerIntCb(hButton0, &button0_WAIT_REPLY_NEW_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		break;
	case WAIT_REPLY_PLAY:
		DisplayState = WAIT_REPLY_PLAY;
		if (PIN_registerIntCb(hActionButton, &actionButton_IDLE_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		if (PIN_registerIntCb(hButton0, &button0_WAIT_REPLY_PLAY_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		break;
	case WAIT_REPLY_SLEEP:
		DisplayState = WAIT_REPLY_SLEEP;
		if (PIN_registerIntCb(hActionButton, &actionButton_IDLE_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		if (PIN_registerIntCb(hButton0, &button0_WAIT_REPLY_SLEEP_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		break;

	case ERROR:
		DisplayState = ERROR;
		if (PIN_registerIntCb(hActionButton, &actionButton_IDLE_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		if (PIN_registerIntCb(hButton0, &button0_ERROR_FXN) != 0) {
			System_abort("Error registering button callback function");
		}
		break;

	case SUCCESS:
		//NOT IMPLEMENTED
		break;
	default:
		break;
	}

}

Void SensorsRefreshDisplay(){
	if(DisplayState == MAIN_1){
		DisplayChanged = true;
	}
}

Void CancelRequests(){
	AwaitingReplyPlay = false;
	AwaitingReplySleep = false;
	AwaitingReplyNew = false;
}

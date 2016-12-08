
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
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
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/mw/display/Display.h>
#include <ti/mw/display/DisplayExt.h>

/* Board Header files */
#include "Board.h"

#include "wireless/comm_lib.h"
#include "sensors/bmp280.h"
#include "Helpers/magnify.h"
#include "Helpers/messages.h"
#include "Aasi.h"


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
	.Active = true
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
#define STACKSIZE 4906
Char taskStack[STACKSIZE];
Char taskCommStack[STACKSIZE];

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
   SEND_NEW,
   ERROR_0
};

// Global display state
enum DisplayStates DisplayState = MAIN_1;
bool DisplayChanged = true;

/* Pin Button1 configured as power button */
static PIN_Handle hPowerButton;
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

/* Handle power button */
Void actionButtonFxn(PIN_Handle handle, PIN_Id pinId) {

    Display_clear(hDisplay);
    Display_close(hDisplay);
    Task_sleep(100000 / Clock_tickPeriod);

	PIN_close(hPowerButton);

    PINCC26XX_setWakeup(cPowerWake);
	Power_shutdown(NULL,0);
}

Void button0_MAIN_0_FXN(PIN_Handle handle, PIN_Id pinId);
Void button0_MENU_0_0_FXN(PIN_Handle handle, PIN_Id pinId);
Void button0_MENU_0_1_FXN(PIN_Handle handle, PIN_Id pinId);
Void button0_MENU_0_2_FXN(PIN_Handle handle, PIN_Id pinId);
Void button0_MENU_1_0_FXN(PIN_Handle handle, PIN_Id pinId);
Void button0_MENU_1_1_FXN(PIN_Handle handle, PIN_Id pinId);
Void button0_MENU_1_2_FXN(PIN_Handle handle, PIN_Id pinId);
Void actionButton_MENU_0_0_FXN(PIN_Handle handle, PIN_Id pinId);
Void actionButton_MENU_0_1_FXN(PIN_Handle handle, PIN_Id pinId);
Void actionButton_MENU_0_2_FXN(PIN_Handle handle, PIN_Id pinId);
Void actionButton_MENU_1_0_FXN(PIN_Handle handle, PIN_Id pinId);
Void actionButton_MENU_1_1_FXN(PIN_Handle handle, PIN_Id pinId);
Void actionButton_MENU_1_2_FXN(PIN_Handle handle, PIN_Id pinId);


/*Main view without donkey*/
Void button0_MAIN_0_FXN(PIN_Handle handle, PIN_Id pinId) {

    DisplayState = MENU_0_0;
    DisplayChanged = true;

    if (PIN_registerIntCb(hPowerButton, &actionButton_MENU_0_0_FXN) != 0) {
    			System_abort("Error registering button callback function");
    }

    if (PIN_registerIntCb(hButton0, &button0_MENU_0_0_FXN) != 0) {
        			System_abort("Error registering button callback function");
    }

}

/*Main view with donkey*/
/*OPEN MENU*/
Void button0_MAIN_1_FXN(PIN_Handle handle, PIN_Id pinId) {

    DisplayState = MENU_1_0;
    DisplayChanged = true;

    if (PIN_registerIntCb(hPowerButton, &actionButton_MENU_1_0_FXN) != 0) {
        			System_abort("Error registering button callback function");
    }
    if (PIN_registerIntCb(hButton0, &button0_MENU_1_0_FXN) != 0) {
    			System_abort("Error registering button callback function");
    }

}

/*Menu without donkey, first option (UUSI) selected*/
/*MOVE TO SECOND MENU OPTION*/
Void button0_MENU_0_0_FXN(PIN_Handle handle, PIN_Id pinId) {
    DisplayState = MENU_0_1;
    DisplayChanged = true;
    if (PIN_registerIntCb(hPowerButton, &actionButton_MENU_0_1_FXN) != 0) {
        			System_abort("Error registering button callback function");
    }
    if (PIN_registerIntCb(hButton0, &button0_MENU_0_1_FXN) != 0) {
    			System_abort("Error registering button callback function");
    }

}

/*Menu without donkey, second option (LEIKI) selected*/
/*MOVE TO THIRD MENU OPTION*/
Void button0_MENU_0_1_FXN(PIN_Handle handle, PIN_Id pinId) {
    DisplayState = MENU_0_0;
    DisplayChanged = true;
    if (PIN_registerIntCb(hPowerButton, &actionButton_MENU_0_2_FXN) != 0) {
            			System_abort("Error registering button callback function");
    }
    if (PIN_registerIntCb(hButton0, &button0_MENU_0_2_FXN) != 0) {
    			System_abort("Error registering button callback function");
    }

}
/*Menu without donkey, third option (TAKAISIN) selected*/
/*MOVE TO FIRST MENU OPTION*/
Void button0_MENU_0_2_FXN(PIN_Handle handle, PIN_Id pinId) {
    DisplayState = MENU_0_2;
    DisplayChanged = true;
    if (PIN_registerIntCb(hPowerButton, &actionButton_MENU_0_0_FXN) != 0) {
            			System_abort("Error registering button callback function");
    }
    if (PIN_registerIntCb(hButton0, &button0_MENU_0_0_FXN) != 0) {
    			System_abort("Error registering button callback function");
    }

}

/*Menu with donkey, first option (NUKU) selected*/
/*MOVE TO SECOND MENU OPTION*/
Void button0_MENU_1_0_FXN(PIN_Handle handle, PIN_Id pinId) {
    DisplayState = MENU_1_1;
    DisplayChanged = true;
    if (PIN_registerIntCb(hPowerButton, &actionButton_MENU_1_1_FXN) != 0) {
            			System_abort("Error registering button callback function");
        }
    if (PIN_registerIntCb(hButton0, &button0_MENU_1_1_FXN) != 0) {
    			System_abort("Error registering button callback function");
    }
}

/*Menu with donkey, second option (MOIKKAA) selected*/
/*MOVE TO THIRD MENU OPTION*/
Void button0_MENU_1_1_FXN(PIN_Handle handle, PIN_Id pinId) {
    DisplayState = MENU_1_2;
    DisplayChanged = true;
    if (PIN_registerIntCb(hPowerButton, &actionButton_MENU_1_2_FXN) != 0) {
            			System_abort("Error registering button callback function");
        }
    if (PIN_registerIntCb(hButton0, &button0_MENU_1_2_FXN) != 0) {
    			System_abort("Error registering button callback function");
    }
}

/*Menu with donkey, third option (TAKAISIN) selected*/
/*MOVE TO FIRST MENU OPTION*/
Void button0_MENU_1_2_FXN(PIN_Handle handle, PIN_Id pinId) {
    DisplayState = MENU_1_0;
    DisplayChanged = true;
    if (PIN_registerIntCb(hPowerButton, &actionButton_MENU_1_0_FXN) != 0) {
            			System_abort("Error registering button callback function");
        }
    if (PIN_registerIntCb(hButton0, &button0_MENU_1_0_FXN) != 0) {
    			System_abort("Error registering button callback function");
    }
}

/*Menu without donkey, first option (UUSI) selected*/
/*SEND NEW DONKEY TO SERVER*/
Void actionButton_MENU_0_0_FXN(PIN_Handle handle, PIN_Id pinId) {
    DisplayState = MAIN_0;
    DisplayChanged = true;
    if (PIN_registerIntCb(hPowerButton, &actionButtonFxn) != 0) {
    			System_abort("Error registering button callback function");
    }
    if (PIN_registerIntCb(hButton0, &button0_MAIN_0_FXN) != 0) {
                			System_abort("Error registering button callback function");
    }
}

/*Menu without donkey, second option (LEIKI) selected*/
/*CALL DONKEY FROM SERVER*/
Void actionButton_MENU_0_1_FXN(PIN_Handle handle, PIN_Id pinId) {
    DisplayState = MAIN_0;
    DisplayChanged = true;
    if (PIN_registerIntCb(hPowerButton, &actionButtonFxn) != 0) {
    			System_abort("Error registering button callback function");
    }
    if (PIN_registerIntCb(hButton0, &button0_MAIN_0_FXN) != 0) {
                			System_abort("Error registering button callback function");
    }
}

/*Menu without donkey, third option (TAKAISIN) selected*/
/*MOVE BACK TO MAIN VIEW*/
Void actionButton_MENU_0_2_FXN(PIN_Handle handle, PIN_Id pinId) {
    DisplayState = MAIN_0;
    DisplayChanged = true;
    if (PIN_registerIntCb(hPowerButton, &actionButtonFxn) != 0) {
    			System_abort("Error registering button callback function");
    }
    if (PIN_registerIntCb(hButton0, &button0_MAIN_0_FXN) != 0) {
                			System_abort("Error registering button callback function");
    }
}

/*Menu with donkey, first option (NUKU) selected*/
/*PUT DONKEY TO SLEEP, SEND STATS TO SERVER*/
Void actionButton_MENU_1_0_FXN(PIN_Handle handle, PIN_Id pinId) {
    DisplayState = MAIN_1;
    DisplayChanged = true;
    if (PIN_registerIntCb(hPowerButton, &actionButtonFxn) != 0) {
    			System_abort("Error registering button callback function");
    }
    if (PIN_registerIntCb(hButton0, &button0_MAIN_1_FXN) != 0) {
                			System_abort("Error registering button callback function");
    }
}

/*Menu with donkey, second option (MOIKKAA) selected*/
/*SEND BROADCAST MESSAGE*/
Void actionButton_MENU_1_1_FXN(PIN_Handle handle, PIN_Id pinId) {
    DisplayState = MAIN_1;
    DisplayChanged = true;

    // Test if previous message send complete
    if(GetTXFlag() == false){
        Send6LoWPAN(IEEE80154_BROADCAST_ADDR, "Terve", 5);
        StartReceive6LoWPAN();
    }
    if (PIN_registerIntCb(hPowerButton, &actionButtonFxn) != 0) {
		System_abort("Error registering button callback function");
    }
    if (PIN_registerIntCb(hButton0, &button0_MAIN_1_FXN) != 0) {
    	System_abort("Error registering button callback function");
    }
}

/*Menu with donkey, third option (TAKAISIN) selected*/
/*MOVE BACK TO MAIN VIEW*/
Void actionButton_MENU_1_2_FXN(PIN_Handle handle, PIN_Id pinId) {
    DisplayState = MAIN_1;
    DisplayChanged = true;
    if (PIN_registerIntCb(hPowerButton, &actionButtonFxn) != 0) {
    			System_abort("Error registering button callback function");
    }
    if (PIN_registerIntCb(hButton0, &button0_MAIN_1_FXN) != 0) {
            			System_abort("Error registering button callback function");
    }
}

/* Communication Task */
Void commFxn(UArg arg0, UArg arg1) {

	uint16_t senderAddr;
	char buffer[80];
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
    		if(Receive6LoWPAN(&senderAddr, buffer, 80) != -1){

				msgType = GetMessageType(buffer);
				switch(msgType){
					// Received greeting -> Send reply, update social
					case HELLO_REC:
						sprintf(buffer, "Terve:%s", aasi.Name );
					    if(GetTXFlag() == false){
					        Send6LoWPAN(senderAddr, buffer, strlen(buffer));
					        StartReceive6LoWPAN();
					    }
					    aasi.Social = aasi.Social + 1;
						break;
					// Received reply -> Update social
					case HELLO_ANS:
						aasi.Social = aasi.Social + 1;
						break;
					default:
						break;

				}
    		}
        }

    	// THIS WHILE LOOP DOES NOT USE Task_sleep
    }
}

Void taskFxn(UArg arg0, UArg arg1) {

//    I2C_Handle      i2c;
//    I2C_Params      i2cParams;

	// Initialize display variables

	uint8_t aasiImageMag[128] = {0};
	magnify(aasi.Image, aasiImageMag);
    const uint32_t imgPalette[] = {0, 0xFFFFFF};
	const tImage aasiImage = {
		.BPP = IMAGE_FMT_1BPP_UNCOMP,
		.NumColors = 2,
		.XSize = 31,
		.YSize = 32,
		.pPalette = imgPalette,
		.pPixel = aasiImageMag
	};

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

    /* Create I2C for usage */
//    I2C_Params_init(&i2cParams);
//    i2cParams.bitRate = I2C_400kHz;
//    i2c = I2C_open(Board_I2C0, &i2cParams);
//    if (i2c == NULL) {
//        System_abort("Error Initializing I2C\n");
//    }
//    else {
//        System_printf("I2C Initialized!\n");
//    }
//
//    // SETUP SENSORS HERE
//    bmp280_setup(&i2c);

    /* Display */
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

    while (1) {
    	if(DisplayChanged == true){
			Display_clear(hDisplay);

			if(DisplayState == MAIN_1){

				Display_print0(hDisplay, 10, 5, aasi.Name);
				// TODO: Tulosta aasin statsit sprinf:n kautta muuttujiin ja korvaa tähän
				Display_print0(hDisplay, 1, 3, "1000    1000");
				Display_print0(hDisplay, 2, 3, "1000    1000");

				GrImageDraw(pContext, &aasiImage, 32, 40);
				GrImageDraw(pContext, &moveImage, 10, 7);
				GrImageDraw(pContext, &sunImage, 55, 7);
				GrImageDraw(pContext, &airImage, 10, 15);
				GrImageDraw(pContext, &socialImage, 55, 15);
				GrLineDraw(pContext,0,24,96,24);
				GrFlush(pContext);
			}
			else if(DisplayState == MAIN_0){
				Display_print0(hDisplay, 10, 4, "Ei aasia :(");
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
			else if(DisplayState == ERROR_0){

			}
			else{
				Display_print0(hDisplay, 3, 3, "MOI");
			}
			DisplayChanged = false;
    	}

    	/*
        // DO SOMETHING HERE
    	bmp280_get_data(&i2c, &pressure, &temperature);
    	sprintf(pres_text, "%.2f hPa", pressure);
    	sprintf(temp_text, "%.2f °C", temperature);
        Display_print0(hDisplay, 4, 1, "Temperature:");
        Display_print0(hDisplay, 5, 1, temp_text);
        Display_print0(hDisplay, 6, 1, "Pressure:");
        Display_print0(hDisplay, 7, 1, pres_text);
        if(pressure >= 1121.40 && pressure <= 1121.50)
        {
        	Display_print0(hDisplay, 8, 1, "1. kerros");
        }
        else if(pressure >= 1120.80 && pressure <= 1120.90)
        {
        	Display_print0(hDisplay, 8, 1, "2. kerros");
        }
        else if(pressure >= 1120.30 && pressure <= 1120.40)
        {
        	Display_print0(hDisplay, 8, 1, "3. kerros");
        }
        else if(pressure <= 1119.90)
        {
        	Display_print0(hDisplay, 8, 1, "4. kerros");
        }
        else
        {
        	Display_print0(hDisplay, 8, 1, "Jossain...");
        } */
    	Task_sleep(1000000 / Clock_tickPeriod);

    }
}

Int main(void) {

    // Task variables
	Task_Handle task;
	Task_Params taskParams;
	Task_Handle taskComm;
	Task_Params taskCommParams;

    // Initialize board
    Board_initGeneral();

	/* Buttons */
	hPowerButton = PIN_open(&sPowerButton, cPowerButton);
	if(!hPowerButton) {
		System_abort("Error initializing button shut pins\n");
	}
	if (PIN_registerIntCb(hPowerButton, &actionButtonFxn) != 0) {
		System_abort("Error registering button callback function");
	}

	hButton0 = PIN_open(&sButton0, cButton0);
		if(!hButton0) {
			System_abort("Error initializing button 0 pins\n");
		}
		if (PIN_registerIntCb(hButton0, &button0_MAIN_1_FXN) != 0) {
			System_abort("Error registering button callback function");
		}

    /* Leds */
    hLed = PIN_open(&sLed, cLed);
    if(!hLed) {
        System_abort("Error initializing LED pin\n");
    }

    /* Task */
    Task_Params_init(&taskParams);
    taskParams.stackSize = STACKSIZE;
    taskParams.stack = &taskStack;
    taskParams.priority=2;

    task = Task_create(taskFxn, &taskParams, NULL);
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
    	System_abort("Task create failed!");
    }


    System_printf("Hello world!\n");
    System_flush();
    
    /* Start BIOS */
    BIOS_start();

    return (0);
}


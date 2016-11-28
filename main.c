
#include <stdio.h>
#include <string.h>

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


/* AASI */
struct Aasi {
    char Name[16] ;
    uint8_t Image[8];
    uint32_t Move;
    uint32_t Sun;
    uint32_t Air;
    uint32_t Social;
};


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
	.Image[7] = 0x18
};


//aasi.Move = 0;

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
   MENU_1_0,
   MENU_1_1,
   MENU_1_2,
   ERROR_0
};

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
Void powerButtonFxn(PIN_Handle handle, PIN_Id pinId) {

    Display_clear(hDisplay);
    Display_close(hDisplay);
    Task_sleep(100000 / Clock_tickPeriod);

	PIN_close(hPowerButton);

    PINCC26XX_setWakeup(cPowerWake);
	Power_shutdown(NULL,0);
}

Void button0Fxn(PIN_Handle handle, PIN_Id pinId) {

    PIN_setOutputValue(hLed, Board_LED0, !PIN_getOutputValue( Board_LED0 ) );
    if (GetRXFlag() == false) {
    	Send6LoWPAN(IEEE80154_SINK_ADDR, "Plort!", 6);
    	StartReceive6LoWPAN();
    }
}

/* Communication Task */
Void commFxn(UArg arg0, UArg arg1) {

	uint16_t senderAddr;
	char payload[80];
	char message[100];

    // Radio to receive mode
	int32_t result = StartReceive6LoWPAN();
	if(result != true) {
		System_abort("Wireless receive mode failed");
	}

    while (1) {
        
    	if (GetRXFlag() == true) {
            
    		if(Receive6LoWPAN(&senderAddr, payload, 80) != -1){
    			sprintf(message, "Recv: %s", payload);
    			System_printf(message);
    			System_flush();
    			Display_print0(hDisplay, 3, 1, message);
    		}
            // WE HAVE A MESSAGE
            // YOUR CODE HERE TO RECEIVE MESSAGE
        }

    	// THIS WHILE LOOP DOES NOT USE Task_sleep
    }
}

Void taskFxn(UArg arg0, UArg arg1) {

    I2C_Handle      i2c;
    I2C_Params      i2cParams;
    
	// Initialize display variables
    const uint32_t imgPalette[] = {0, 0xFFFFFF};
	const tImage aasiImage = {
		.BPP = IMAGE_FMT_1BPP_UNCOMP,
		.NumColors = 2,
		.XSize = 1,
		.YSize = 8,
		.pPalette = imgPalette,
		.pPixel = aasi.Image
	};
	enum DisplayStates displayState = MAIN_0;

    /* Create I2C for usage */
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;
    i2c = I2C_open(Board_I2C0, &i2cParams);
    if (i2c == NULL) {
        System_abort("Error Initializing I2C\n");
    }
    else {
        System_printf("I2C Initialized!\n");
    }

    // SETUP SENSORS HERE
    bmp280_setup(&i2c);

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

    	if(displayState == MAIN_1){

    		GrLineDraw(pContext,0,10,96,10);
    		GrImageDraw(pContext, &aasiImage, 30, 30);
    		GrFlush(pContext);
    		Display_print0(hDisplay, 10, 4, aasi.Name);
    	}
    	if(displayState == MAIN_0){
    		Display_print0(hDisplay, 10, 4, "Ei aasia :(");
    	}
    	else{
    		Display_print0(hDisplay, 3, 3, "MOI");
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
	if (PIN_registerIntCb(hPowerButton, &powerButtonFxn) != 0) {
		System_abort("Error registering button callback function");
	}

	hButton0 = PIN_open(&sButton0, cButton0);
		if(!hButton0) {
			System_abort("Error initializing button 0 pins\n");
		}
		if (PIN_registerIntCb(hButton0, &button0Fxn) != 0) {
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


/*
 * tmp007.c
 *
 *  Created on: 28.9.2016
 *  Author: Teemu Leppanen / UBIComp / University of Oulu
 *
 *  Datakirja: http://www.ti.com/lit/ds/symlink/tmp007.pdf
 */

#include <xdc/runtime/System.h>
#include <string.h>
#include "Board.h"
#include "tmp007.h"

I2C_Transaction i2cTransaction;
char txBuffer[4];
char rxBuffer[2];

void tmp007_setup(I2C_Handle *i2c) {

	System_printf("TMP007: Config not needed!\n");
    System_flush();
}

double tmp007_get_data(I2C_Handle *i2c) {

	double temperature = 0.0;

	/* FILL OUT THIS DATA STRUCTURE TO GET TEMPERATURE DATA
    i2cTransaction.slaveAddress = ...
    i2cTransaction.writeBuf = ...
    i2cTransaction.writeCount = ...
    i2cTransaction.readBuf = ...
    i2cTransaction.readCount = ...
	*/

	if (I2C_transfer(*i2c, &i2cTransaction)) {

		// HERE YOU GET THE TEMPERATURE VALUE FROM RXBUFFER
		// ACCORDING TO DATASHEET

	} else {

		System_printf("TMP007: Data read failed!\n");
		System_flush();
	}

	return temperature;
}



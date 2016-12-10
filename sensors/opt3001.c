/*
 * opt3001.c
 *
 *  Created on: 22.7.2016
 *  Author: Teemu Leppanen / UBIComp / University of Oulu
 *
 *  Datakirja: http://www.ti.com/lit/ds/symlink/opt3001.pdf
 */

#include <string.h>
#include <math.h>

#include <xdc/runtime/System.h>

#include "opt3001.h"
#include "Board.h"

I2C_Transaction i2cTransaction;
char txBuffer[4];
char rxBuffer[2];

void opt3001_setup(I2C_Handle *i2c) {

    i2cTransaction.slaveAddress = Board_OPT3001_ADDR;
    txBuffer[0] = OPT3001_REG_CONFIG;
    txBuffer[1] = 0xCE; // continuous mode s.22
    txBuffer[2] = 0x02;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 3;
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;

    if (I2C_transfer(*i2c, &i2cTransaction)) {

        System_printf("OPT3001: Config write ok\n");
    } else {
        System_printf("OPT3001: Config write failed!\n");
    }
    System_flush();

}

double opt3001_get_data(I2C_Handle *i2c) {

	uint16_t e=0;
	double lux;
	uint16_t light_r;
	uint8_t light_e;

	/* Read sensor state */
	i2cTransaction.slaveAddress = Board_OPT3001_ADDR;
	txBuffer[0] = OPT3001_REG_CONFIG;
	i2cTransaction.writeBuf = txBuffer;
	i2cTransaction.writeCount = 1;
	i2cTransaction.readBuf = rxBuffer;
	i2cTransaction.readCount = 2;

	if (I2C_transfer(*i2c, &i2cTransaction)) {

		e = (rxBuffer[0] << 8) | rxBuffer[1];

	} else {

		e = 0;

		System_printf("OPT3001: Config read failed!\n");
		System_flush();
	}

	/* Data available? */
	if (e & OPT3001_DATA_READY) {

		// FILL OUT THIS DATA STRUCTURE TO GET LUX DATA
		txBuffer[0] = OPT3001_REG_RESULT;
	    i2cTransaction.slaveAddress = Board_OPT3001_ADDR;
	    i2cTransaction.writeBuf = txBuffer;
	    i2cTransaction.writeCount = 1;
	    i2cTransaction.readBuf = rxBuffer;
	    i2cTransaction.readCount = 2;


		if (I2C_transfer(*i2c, &i2cTransaction)) {

			// HERE YOU NEED TO GET THE LUX VALUE FROM RXBUFFER
	    	// ACCORDING TO DATASHEET
			light_r = (rxBuffer[1] | ((rxBuffer[0] & 0xF) << 8));
			light_e = rxBuffer[0] >> 4;
			light_e = pow(2, light_e);
			lux = (double)(0.01 * light_e * light_r);


		} else {

			System_printf("OPT3001: Data read failed!\n");
			System_flush();
		}
	}

	// FIXED THIS
	return lux;
}

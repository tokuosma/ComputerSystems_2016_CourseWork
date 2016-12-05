#include <inttypes.h>
#include "magnify.h"

// Magnifies a 8*8 source image data to 32*32 image.
void magnify(uint8_t  source [8], uint8_t target[128]){
  int i;
  int j;
  //Create temporary array for magnified rows
  uint32_t tempArr [32] = {0};
  for(i = 0; i < 8; i++){
	uint32_t temp = magnify_row(source[i]);
	for(j = 0; j < 4; j++){
		tempArr[i*4 + j] = temp;
	}
  }
  //Copy magnified rows to target array in 8-bit chunks
  for(i = 0; i < 32; i++){
	  for(j = 0; j < 4; j++){
		  uint8_t val = ((tempArr[i]) >> (8 * (3 - j)));
		  target[4*i + j] = val;
	  }
  }
  return;
}


uint32_t magnify_row(uint8_t row_int){
  uint32_t mag_int = 0;
  if((row_int & 1) != 0){
    mag_int = mag_int | 0x0000000F;
  }
  if((row_int & 2) != 0){
    mag_int = mag_int | 0x000000F0;
  }
  if((row_int & 4) != 0){
    mag_int = mag_int | 0x00000F00;
  }
  if((row_int & 8) != 0){
    mag_int = mag_int | 0x0000F000;
  }
  if((row_int & 16) != 0){
    mag_int = mag_int | 0x000F0000;
  }
  if((row_int & 32) != 0){
    mag_int = mag_int | 0x00F00000;
  }
  if((row_int & 64) != 0){
    mag_int = mag_int | 0x0F000000;
  }
  if((row_int & 128) != 0){
    mag_int = mag_int | 0xF0000000;
  }

  return mag_int;
}

#include <inttypes.h>
#include "magnify.h"

void magnify(uint8_t  source [8], uint8_t target[32][4], int sizeY){
  int i;
  for(i = 0; i < sizeY - 3; i++){
	uint32_t temp = magnify_row(source[i]);
	split_mag_row(temp, target[4*i]);
	split_mag_row(temp, target[4*i + 1]);
	split_mag_row(temp, target[4*i + 2]);
	split_mag_row(temp, target[4*i + 3]);
  }
  return;
}

void split_mag_row(uint32_t source, uint8_t targetArr[4]){
	int i;
	uint8_t mask = 0xFF;
	for(i = 0; i < 4; i++){
		uint8_t temp = (source >> (3-i) * 8) & mask;
		targetArr[i] = temp;
	}
}

uint32_t magnify_row(uint8_t row_int){
  uint32_t mag_int = 0;
  if(row_int & 1){
    mag_int = mag_int | 0x0000000F;
  }
  if(row_int & 2){
    mag_int = mag_int | 0x000000F0;
  }
  if(row_int & 4){
    mag_int = mag_int | 0x00000F00;
  }
  if(row_int & 8){
    mag_int = mag_int | 0x0000F000;
  }
  if(row_int & 16){
    mag_int = mag_int | 0x000F0000;
  }
  if(row_int & 32){
    mag_int = mag_int | 0x00F00000;
  }
  if(row_int & 64){
    mag_int = mag_int | 0x0F000000;
  }
  if(row_int & 128){
    mag_int = mag_int | 0xF0000000;
  }

  return mag_int;
}

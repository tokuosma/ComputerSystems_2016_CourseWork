

#ifndef MAGNIFY_H_
#define MAGNIFY_H_

#include <inttypes.h>

void magnify(uint8_t source[8], uint8_t target[32][4], int sizeY);
void split_mag_row(uint32_t source, uint8_t targetArr[4]);
uint32_t magnify_row(uint8_t row_int);

#endif

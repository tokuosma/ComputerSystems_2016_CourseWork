#ifndef AASI_H_
#define AASI_H_

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdio.h>

struct Aasi {
    char Name[16] ;
    uint8_t Image[8];
    uint32_t Move;
    uint32_t Sun;
    uint32_t Air;
    uint32_t Social;
    bool Active;
};

void serialize_aasi_new(struct Aasi aasi, char * buffer);
void serialize_aasi_sleep(struct Aasi aasi, char * buffer);
void serialize_aasi_play(char * buffer);

struct Aasi deserialize_aasi_play(char * msg_raw);

#endif

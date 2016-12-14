#include "Aasi.h"

//int main(){
//    struct Aasi aasi = {
//        .Name = "Duffy",
//        .Move = 0,
//        .Sun = 0,
//        .Air = 0,
//        .Social = 0,
//        .Image[0] = 0x81,
//        .Image[1] = 0x42,
//        .Image[2] = 0x7E,
//        .Image[3] = 0xA5,
//        .Image[4] = 0x81,
//        .Image[5] = 0x5A,
//        .Image[6] = 0x24,
//        .Image[7] = 0x18,
//        .Active = true
//    };
//
//    char buff[200];
//    serialize_aasi_new(aasi, buff);
//    printf("%s",buff);
//    serialize_aasi_sleep(aasi,buff);
//    printf("%s",buff);
//    char msg[] = "OK:11,36,59,66,66,66,59,0,1,2,3,4,Ekamax\n";
//    aasi = deserialize_aasi_play(msg);
//    serialize_aasi_new(aasi, buff);
//    printf("%s",buff);
//    serialize_aasi_sleep(aasi,buff);
//    printf("%s",buff);
//    return 0;
//}

const char AASI_NAME[16] = "Duffy";

/*Generate new donkey message and print to buffer*/
void  serialize_aasi_new(struct Aasi aasi, char * buffer){

	char temp [80];
	sprintf(temp, "Uusi:%d,%d,%d,%d,%d,%d,%d,%d,%s\n",
           aasi.Image[0],
           aasi.Image[1],
           aasi.Image[2],
           aasi.Image[3],
           aasi.Image[4],
           aasi.Image[5],
           aasi.Image[6],
           aasi.Image[7],
           aasi.Name);
	memcpy(buffer, temp, strlen(temp));
}

/*Generate play message and save to buffer*/
void serialize_aasi_play(char * buffer){
	sprintf(buffer, "Leiki:%s\n", AASI_NAME);
}

/*Generate sleep message and save to buffer*/
void serialize_aasi_sleep(struct Aasi aasi, char * buffer){
    sprintf(buffer, "Nuku:%d,%d,%d,%d,%s\n",
            aasi.Move,
            aasi.Sun,
            aasi.Air,
            aasi.Social,
            aasi.Name);
}

/*Deserialize and return new donkey*/
struct Aasi deserialize_aasi_play( char * msg_raw){


    const char sep1 = ':';
    const char sep2[2] = ",";
    // Remove 'OK:'
    char * msg = (strchr(msg_raw, sep1)+1);

    char * token;
    uint8_t image[8] = {0};
    uint32_t move;
    uint32_t sun;
    uint32_t air;
    uint32_t social;
    char name[16] = ""; 

    token = strtok(msg, sep2);
    image[0] = atoi(token);
    token = strtok(NULL, sep2);
    image[1] = atoi(token);
    token = strtok(NULL, sep2);
    image[2] = atoi(token);
    token = strtok(NULL, sep2);
    image[3] = atoi(token);
    token = strtok(NULL, sep2);
    image[4] = atoi(token);
    token = strtok(NULL, sep2);
    image[5] = atoi(token);
    token = strtok(NULL, sep2);
    image[6] = atoi(token);
    token = strtok(NULL, sep2);
    image[7] = atoi(token);
    token = strtok(NULL, sep2);
    move = atoi(token);
    token = strtok(NULL, sep2);
    sun = atoi(token);
    token = strtok(NULL, sep2);
    air = atoi(token);
    token = strtok(NULL, sep2);   
    social = atoi(token);
    token = strtok(NULL, sep2);
    sprintf(name, "%s", token);

    struct Aasi aasi = {
        .Move = move,
        .Sun = sun,
        .Air = air,
        .Social = social,
        .Image[0] = image[0],
        .Image[1] = image[1],
        .Image[2] = image[2],
        .Image[3] = image[3],
        .Image[4] = image[4],
        .Image[5] = image[5],
        .Image[6] = image[6],
        .Image[7] = image[7],
        .Active = true
    };
    memcpy(aasi.Name, name, strlen(token) + 1);

    return aasi;
}



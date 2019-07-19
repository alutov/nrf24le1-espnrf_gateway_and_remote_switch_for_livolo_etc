#ifndef H_RFLINK
#define H_RFLINK

#include <Arduino.h>
#include "Common.h"

/*********************************************************************************
 * RFLink settings
/*********************************************************************************/

// if less than that, rflink line will be ignored
#define RFLINK_PACKET_MIN_SIZE 8

// main input / output buffers
extern char BUFFER [BUFFER_SIZE];
extern char JSON   [BUFFER_SIZE];

// message builder buffers
extern char MQTT_NAME[MAX_DATA_LEN];
extern char MQTT_ID  [MAX_DATA_LEN];
extern char MQTT_PRFX[MAX_PRFX_LEN];
extern bool f_lfwnrf;
extern bool f_rfwnrf;
extern bool f_ffwnrf;
extern bool f_sfwnrf;
extern int  f_tfwnrf;
//extern char FIELD_BUF[MAX_DATA_LEN];


/*********************************************************************************
 * const strings used in helper functions
/*********************************************************************************/


/*********************************************************************************
 * RFLink functions
/*********************************************************************************/
void readRfLinkPacket(char* line);

#endif

/*********************************************************************************
 * RFLink parser functions
/*********************************************************************************/
#include "Rflink.h"

/**
 * Read RFLink packet (line terminated by \n) and
 * - extract message name / driver (eg: OregonV1) to MQTT_NAME
 * - create a JSON message with all encountered fields
 */

void readRfLinkPacket(char* line) {
        // check len and ignore bad packet (broken serial, etc.)
      if(strlen(line) < RFLINK_PACKET_MIN_SIZE) return;
        int i = 0; // read nrf1/
        int j = 0;
        while(i < 5) {
                MQTT_PRFX[j] = line[i];
                i++; j++;
        }
        MQTT_PRFX[j] = '\0';
        //check prefix "nrf1/"
        if ( strcmp(MQTT_PRFX,MQTT_PUBLISH_TOPIC) != 0) return;
        j=0;
        while(i < 8) {
                MQTT_PRFX[j] = line[i];
                i++; j++;
        }
        MQTT_PRFX[j] = '\0';
        // check if next 16 bytes of fw stored in NRF24LE1 memory
        if ( strcmp(MQTT_PRFX,":Ok") == 0) {
          f_tfwnrf = 0; f_rfwnrf = true; return;
          }
        if ( strcmp(MQTT_PRFX,":Er") == 0) {
          f_tfwnrf = 0; f_lfwnrf = false; f_rfwnrf = true;
          f_ffwnrf = true; f_sfwnrf = true; return;
          }

        	
        	i = 5; // ignore nrf1/
        	j = 0;

        // get name (begins at char 5)
        while(line[i] != ' ' && i < BUFFER_SIZE && j < MAX_DATA_LEN) {
                MQTT_NAME[j] = line[i];
				        i++; j++;
        }
        MQTT_NAME[j] = '\0';
        if ( j > 32 ) return;
        // ends string correctly
        
        // get value string
                j=0;
                i++; // Skip ;
                while(line[i] > 31 && i < BUFFER_SIZE && j < BUFFER_SIZE) {
                        JSON[j] = line[i];
                        i++; j++;
                }
        JSON[j]='\0';
        if ( j > 32 ) return;
        MQTT_ID[0]='0'; MQTT_ID[1]='\0';
                return;
}

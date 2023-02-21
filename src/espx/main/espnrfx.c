/* 
****************************************************************
	espnrf main
	Lutov Andrey  Donetsk
Use for compilation ESP-IDF Programming Guide:
https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/
****************************************************************
*/
#define AP_VER "2023.02.17"
#include "espnrf.h"

typedef struct  {        // Preconfigured commands to show on web interface
  char text[MAX_DATA_LEN];      // button text
  char command[BUFFER_SIZE];    // command to send
} USER_CMD_STRUCT;

const USER_CMD_STRUCT USER_CMDs[] = {    // Configure commands to show on web interface
  { "Get"        ,  "get 1 1"                               },
  { "Add"        ,  "add 1 1 test"                          },
  { "Del"        ,  "del 1 1"                               },
  { "Ldfw"       ,  "ldfw 0"                                },
  { "Rfch"       ,  "rfch"                                  },
  { "Alrfch"     ,  "alrfch"                                },
  { "Rfpwr"      ,  "rfpwr 4"                               },
  { "Rfid"       ,  "rfid"                                  },
  { "Rcprt"      ,  "rcprt 1 1"                             },
  { "Stval"      ,  "stval 1 2"                             },
};


//**************** my common proc *******************
// bin byte to hex char string
void bin2hex(const unsigned char *bin, char *out, size_t len)
{
	size_t  i;
	if (bin == NULL || out == NULL || len == 0) return;
	for (i=0; i<len; i++) {
		out[i*2]   = "0123456789abcdef"[bin[i] >> 4];
		out[i*2+1] = "0123456789abcdef"[bin[i] & 0x0F];
	}
	out[len*2] = '\0';
}

// parse uri par=value string like this:
// swfid=wifiname&swfpsw=wifipassword&smqsrv=192.168.1.10&smqid=esp&
// smqpsw=esp&devnam=&rlight=255&glight=255&blight=255&chk2=2
// find par *key in string *cin(isize) & copy value to *out buffer(osize) 
void parsuri(char *cin, char *cout, char *ckey, int isize, int osize)
{
	int i = 0;
	int j = 0;
	char found = 0;
	char a;
	char b;
	char c;
	while ((i < isize) && (!found)) {
	a = cin[i];
	b = ckey[j];
	i++;
	j++;
	if (a != b) {
	if ((a == 0x3d) && (b == 0)) {                 //0x3d-> =
	found = 1;
	cout[0] = 0;
	}
	if (a < 0x20) i = isize;
	j = 0;
	}
	}	
	j = 0;
	while ((i < isize) && (j < osize) && (found)) {
	a = cin[i];
	if ((a > 0x1f) && (a != 0x26)) {       //0x26-> &
	if (a == 0x25) {
	i++;
	a = cin[i];
	b =  (a <= '9') ? (a - '0') : (a - 'A' + 10);
	b = (b & 0xf)*16;
	i++;
	a = cin[i];
	c =  (a <= '9') ? (a - '0') : (a - 'A' + 10);
	c = (c & 0xf) + b;
	cout[j] = c;
	} else if (a == 0x2b) cout[j] = 0x20;
	else cout[j] = cin[i];
	i++; j++;
	cout[j] = 0;
	} else found = 0;
	}
}

//return offset after founded string 0 if not found
int parsoff(char *cin, char *ckey, int isize)
{
	int i = 0;
	int j = 0;
	char found = 0;
	char a;
	char b;
	while ((i < isize) && (!found)) {
	a = cin[i];
	b = ckey[j];
	i++;
	j++;
	if (a != b) {
	if (b == 0) {
	found = 1;
	i--;
	}
	j = 0;
	}
	}	
	if (!found) i = 0;
	return i;
}

//my string copy include dest buf limit
void mystrcpy(char *cout, char *cin, int osize)
{
	int i = 0;
	char a;
	while (i < osize) {
	a = cin[i];
	if  (a > 0x1f) {
	cout[i] = a;
	i++;
	cout[i] = 0;
	} else {
	i = osize;
	}		
	}
}


// size control case insensitive strings compare 
bool incascmp(char *cain, char *cbin, int size)
{
	int i = 0;
	char ca;
	char cb;
	bool result = 0;
	while ((i < size) && (!result)) {
	ca = cain[i];
	cb = cbin[i];
	i++;
	if (ca >= 'A' && ca <= 'Z')
	ca = 'a' + (ca - 'A');
	if (cb >= 'A' && cb <= 'Z')
	cb = 'a' + (cb - 'A');
	if ((ca==0) || (ca != cb)) result = 1;
	}
	if (cain[i] != 0) result = 1;
	return result;
}

// size control case sensitive strings compare 
bool inccmp(char *cain, char *cbin, int size)
{
	int i = 0;
	char ca;
	char cb;
	bool result = 0;
	while ((i < size) && (!result)) {
	ca = cain[i];
	cb = cbin[i];
	i++;
	if ((ca==0) || (ca != cb)) result = 1;
	}
	if (cain[i] != 0) result = 1;
	return result;
}

// store uptime string in *cout buffer
void uptime_string_exp(char *cout)
{
	char buff[16];

//	time_t now;
//	time(&now);
//	int minutes = now / 60;

	int minutes = tuptime / 7500;
	int days = minutes / 1440;
	minutes = minutes % 1440;
	int hrs = minutes / 60;
	minutes = minutes % 60;
       	itoa(days,buff,10);
	strcpy (cout,buff);
	strcat (cout," days ");
       	itoa(hrs,buff,10);
	strcat (cout,buff);
	strcat (cout," hours ");
       	itoa(minutes,buff,10);
	strcat (cout,buff);
	strcat (cout," minutes");
}

  /**
     Time functions
  */
void hw_timer_callback(void *arg)
{
	tuptime++;

	if (f_tfwnrf) {
	f_tfwnrf--;
	if  (!f_tfwnrf) {
        f_lfwnrf = false;
        f_rfwnrf = true;    // no waiting for response
        f_ffwnrf = true;    // load fw error
        f_sfwnrf = true;
	}
	}
	if (t_tsonof) t_tsonof--;
	if (n_button1 < 50) n_button1++;
	if (n_button2 < 50) n_button2++;
	if (n_button3 < 50) n_button3++;
	if (n_button4 < 50) n_button4++;

	if (wmode == 1) {
	if (NAMONOF1[0]) {
	if (!gpio_get_level(BUTTON1)) {
	n_button1 = 0;
	if (!f_button1) {
	f_button1 = true; f_snfst1 = true; 
	gpio_set_level(RELAY1,!gpio_get_level(RELAY1));
	}
	}
	if (gpio_get_level(BUTTON1)) {
	if ((f_button1) && (n_button1 > 5)) {
	f_button1 = false; n_button1 = 0;
	}
	}
	}

	if (NAMONOF2[0]) {
	if (!gpio_get_level(BUTTON2)) {
	n_button2 = 0;
	if (!f_button2) {
	f_button2 = true; f_snfst2 = true; 
	gpio_set_level(RELAY2,!gpio_get_level(RELAY2));
	}
	}
	if (gpio_get_level(BUTTON2)) {
	if ((f_button2) && (n_button2 > 5)) {
	f_button2 = false; n_button2 = 0;
	}
	}
	}


	if (NAMONOF3[0]) {
	if (!gpio_get_level(BUTTON3)) {
	n_button3 = 0;
	if (!f_button3) {
	f_button3 = true; f_snfst3 = true; 
	gpio_set_level(RELAY3,!gpio_get_level(RELAY3));
	}
	}
	if (gpio_get_level(BUTTON3)) {
	if ((f_button3) && (n_button3 > 5)) {
	f_button3 = false; n_button3 = 0;
	}
	}
	}


	if (NAMONOF4[0]) {
	if (!gpio_get_level(BUTTON4)) {
	n_button4 = 0;
	if (!f_button4) {
	f_button4 = true; f_snfst4 = true; 
	gpio_set_level(RELAY4,!gpio_get_level(RELAY4));
	}
	}
	if (gpio_get_level(BUTTON4)) {
	if ((f_button4) && (n_button4 > 5)) {
	f_button4 = false; n_button4 = 0;
	}
	}
	}
	} else if (wmode == 2) {
	if (NAMONOF1[0]) {
	if (tdelcover) {
	tdelcover--;
	if  (!tdelcover) {
	f_snfst1 = true;
	if (lvout ^ gpio_get_level(RELAY1)) {
	gpio_set_level(RELAY1, lvout);
	}
	}
	}

	if (lvout ^ gpio_get_level(BUTTON1)) {
	n_button1 = 0;
	if (!f_button1) {
	f_button1 = true; f_snfst1 = true; 
	lockcount++;
	tdelcover = 250;
	}
	} else {
	if ((f_button1) && (n_button1 > 5)) {
	f_button1 = false; n_button1 = 0;
	}
	}

	}
	} else if ((wmode == 3) && NAMONOF1[0]) {
	if ((lvout ^ gpio_get_level(RELAY1)) && (lvout ^ gpio_get_level(RELAY2))) {
	gpio_set_level(RELAY1, lvout);
	gpio_set_level(RELAY2, lvout);
	crcvpos = 254;
	tdelcover = 20;
	} else if (tdelcover) {
	tdelcover--;
	if  (!tdelcover) {
	if (lvout ^ gpio_get_level(RELAY1)) {
	gpio_set_level(RELAY1, lvout);
	tdelcover = 20;
	crcvpos = 100;
	prcvpos = 255;
	} else if (lvout ^ gpio_get_level(RELAY2)) {
	gpio_set_level(RELAY2, lvout);
	tdelcover = 20;
	crcvpos = 0;
	prcvpos = 255;
	}
	}
	}
	if (!gpio_get_level(BUTTON2)) {
	n_button2 = 0;
	if (!f_button2) {
	f_button2 = true; 
	if (stcvpos != 0) stcvpos = 0;
	else stcvpos = 254;
	}
	} else {
	if ((f_button2) && (n_button2 > 5)) {
	f_button2 = false; n_button2 = 0;
	}
	if (!gpio_get_level(BUTTON1)) {
	n_button1 = 0;
	if (!f_button1) {
	f_button1 = true; 
	if (stcvpos != 100) stcvpos = 100;
	else stcvpos = 254;
	}
	} else {
	if ((f_button1) && (n_button1 > 5)) {
	f_button1 = false; n_button1 = 0;
	}
	}
	}
	if (crcvpos != stcvpos) {
	if (stcvpos == 0) {
	if (lvout ^ gpio_get_level(RELAY1)) {
	gpio_set_level(RELAY1, lvout);
	tdelcover = 25;
	crcvpos = 253;
	prcvpos = 255;
	} else if (!(lvout ^ gpio_get_level(RELAY2))) { 
	if (!tdelcover)	{
	gpio_set_level(RELAY2, (lvout ^ 1));
	if (CDELAY) tdelcover = CDELAY * 125;
	else tdelcover = 22500;
	crcvpos = 252;
	prcvpos = 255;
	}
	}
	} else if (stcvpos == 100) {

	if (lvout ^ gpio_get_level(RELAY2)) {
	gpio_set_level(RELAY2, lvout);
	tdelcover = 25;
	crcvpos = 253;
	prcvpos = 255;
	} else if (!(lvout ^ gpio_get_level(RELAY1))) { 
	if (!tdelcover)	{
	gpio_set_level(RELAY1, (lvout ^ 1));
	if (CDELAY) tdelcover = CDELAY * 125;
	else tdelcover = 22500;
	crcvpos = 251;
	prcvpos = 255;
	}
	}

	} else if ((lvout ^ gpio_get_level(RELAY1)) || (lvout ^ gpio_get_level(RELAY2))){
	gpio_set_level(RELAY1, lvout);
	gpio_set_level(RELAY2, lvout);
	tdelcover = 25;
	prcvpos = 255;
	crcvpos = 254;
	}
	}


	}

}



//******************* Mqtt **********************
//static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    // your_context_t *context = event->context;
	int topoff = 0;
	int topoffcv = 0;
	char tbuff[128];
	char ttopic[128];
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
//	mqttConnected = false;
//	ESP_LOGI(TAG,"MQTT_EVENT_CONNECTED");
	iprevRssiESP = 0;
	f_snfst1 = true;
	f_snfst2 = true;
	t_tsonof = 2500;
	f_snfsnd1 = true;
	f_snfsnd2 = true;
	f_snfdel = false;
	char llwtt[64];
	char llwtd[2048];
	char buff[16];
	strcpy(llwtt,MQTT_BASE_TOPIC);
	strcat(llwtt,"/status");
	esp_mqtt_client_publish(client, llwtt, "online", 0, 1, 1);
	tcpip_adapter_ip_info_t ipInfo;
	char wbuff[256];
	memset(wbuff,0,32);
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
	sprintf(wbuff, "%d.%d.%d.%d", IP2STR(&ipInfo.ip));
	strcpy(llwtt,MQTT_BASE_TOPIC);
	strcat(llwtt,"/ip");
	esp_mqtt_client_publish(client, llwtt, wbuff, 0, 1, 1);

	strcpy(llwtt,MQTT_BASE_TOPIC);
	strcat(llwtt,"/#");
	esp_mqtt_client_subscribe(mqttclient, llwtt, 0);
	mqttConnected = true;
	NumMqConn++;
	if (!NumMqConn) NumMqConn--;
	if (FDHass && tESP8266Addr[0]) {
//
	strcpy(llwtt,"homeassistant/sensor/");
	strcat(llwtt,MQTT_BASE_TOPIC);
	strcat(llwtt,"/1x");
	strcat(llwtt,tESP8266Addr);
	strcat(llwtt,"/config");
	strcpy(llwtd,"{\"name\":\"espNRF");
	if (espnrfnum)  {
	itoa(espnrfnum,buff,10);
	strcat(llwtd, buff);
	}
	strcat(llwtd,".Rssi\",\"icon\":\"mdi:wifi\",\"uniq_id\":\"Rssi_");
	strcat(llwtd,tESP8266Addr);
	strcat(llwtd,"\",\"device\":{\"identifiers\":[\"espNRF_");
	strcat(llwtd,tESP8266Addr);
	strcat(llwtd,"\"],\"name\":\"espNRF");
	if (espnrfnum)  {
	strcat(llwtd, ".");
	itoa(espnrfnum,buff,10);
	strcat(llwtd, buff);
	}
	strcat(llwtd,"\",\"model\":\"espNRF\",\"sw_version\":\"");
	strcat(llwtd,AP_VER);
	if (wbuff[0]) {
	strcat(llwtd,"\",\"configuration_url\":\"http://");
	strcat(llwtd,wbuff);
	}
	strcat(llwtd,"\",\"manufacturer\":\"Espressif & Nordic Semiconductor\"},");
	strcat(llwtd,"\"device_class\":\"signal_strength\",\"state_class\":\"measurement\",\"state_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/rssi\",\"unit_of_meas\":\"dBm\",\"availability_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/status\"}");
	esp_mqtt_client_publish(mqttclient, llwtt, llwtd, 0, 1, 1);
//

	if (wmode == 1) {
	if (NAMONOF1[0] && NAMONOF1[0] != 0x5f) {
	strcpy(llwtt,"homeassistant/switch/");
	strcat(llwtt,MQTT_BASE_TOPIC);
	strcat(llwtt,"/1x");
	strcat(llwtt,tESP8266Addr);
	strcat(llwtt,"/config");
	strcpy(llwtd,"{\"name\":\"");
//	strcat(llwtd,MQTT_BASE_TOPIC);
//	strcat(llwtd,".");
	strcat(llwtd,NAMONOF1);
	strcat(llwtd,"\",\"icon\":\"mdi:electric-switch\",\"uniq_id\":\"Switch_1.");
	strcat(llwtd,tESP8266Addr);
	strcat(llwtd,"\",\"device\":{\"identifiers\":[\"espNRF_");
	strcat(llwtd,tESP8266Addr);
	strcat(llwtd,"\"],\"name\":\"espNRF");
	if (espnrfnum)  {
	strcat(llwtd, ".");
	itoa(espnrfnum,buff,10);
	strcat(llwtd, buff);
	}
	strcat(llwtd,"\",\"model\":\"espNRF\",\"sw_version\":\"");
	strcat(llwtd,AP_VER);
	if (wbuff[0]) {
	strcat(llwtd,"\",\"configuration_url\":\"http://");
	strcat(llwtd,wbuff);
	}
	strcat(llwtd,"\",\"manufacturer\":\"Espressif & Nordic Semiconductor\"},\"command_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/");
	strcat(llwtd,NAMONOF1);
	strcat(llwtd,"\",\"state_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/");
	strcat(llwtd,NAMONOF1);
	strcat(llwtd,"\",\"availability_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/status\"}");
	esp_mqtt_client_publish(mqttclient, llwtt, llwtd, 0, 1, 1);
//
	}
	if (NAMONOF2[0] && NAMONOF2[0] != 0x5f) {
	strcpy(llwtt,"homeassistant/switch/");
	strcat(llwtt,MQTT_BASE_TOPIC);
	strcat(llwtt,"/2x");
	strcat(llwtt,tESP8266Addr);
	strcat(llwtt,"/config");
	strcpy(llwtd,"{\"name\":\"");
//	strcat(llwtd,MQTT_BASE_TOPIC);
//	strcat(llwtd,".");
	strcat(llwtd,NAMONOF2);
	strcat(llwtd,"\",\"icon\":\"mdi:electric-switch\",\"uniq_id\":\"Switch_2.");
	strcat(llwtd,tESP8266Addr);
	strcat(llwtd,"\",\"device\":{\"identifiers\":[\"espNRF_");
	strcat(llwtd,tESP8266Addr);
	strcat(llwtd,"\"],\"name\":\"espNRF");
	if (espnrfnum)  {
	strcat(llwtd, ".");
	itoa(espnrfnum,buff,10);
	strcat(llwtd, buff);
	}
	strcat(llwtd,"\",\"model\":\"espNRF\",\"sw_version\":\"");
	strcat(llwtd,AP_VER);
	if (wbuff[0]) {
	strcat(llwtd,"\",\"configuration_url\":\"http://");
	strcat(llwtd,wbuff);
	}
	strcat(llwtd,"\",\"manufacturer\":\"Espressif & Nordic Semiconductor\"},\"command_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/");
	strcat(llwtd,NAMONOF2);
	strcat(llwtd,"\",\"state_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/");
	strcat(llwtd,NAMONOF2);
	strcat(llwtd,"\",\"availability_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/status\"}");
	esp_mqtt_client_publish(mqttclient, llwtt, llwtd, 0, 1, 1);
//
	}
	if (NAMONOF3[0] && NAMONOF3[0] != 0x5f) {
	strcpy(llwtt,"homeassistant/switch/");
	strcat(llwtt,MQTT_BASE_TOPIC);
	strcat(llwtt,"/3x");
	strcat(llwtt,tESP8266Addr);
	strcat(llwtt,"/config");
	strcpy(llwtd,"{\"name\":\"");
//	strcat(llwtd,MQTT_BASE_TOPIC);
//	strcat(llwtd,".");
	strcat(llwtd,NAMONOF3);
	strcat(llwtd,"\",\"icon\":\"mdi:electric-switch\",\"uniq_id\":\"Switch_3.");
	strcat(llwtd,tESP8266Addr);
	strcat(llwtd,"\",\"device\":{\"identifiers\":[\"espNRF_");
	strcat(llwtd,tESP8266Addr);
	strcat(llwtd,"\"],\"name\":\"espNRF");
	if (espnrfnum)  {
	strcat(llwtd, ".");
	itoa(espnrfnum,buff,10);
	strcat(llwtd, buff);
	}
	strcat(llwtd,"\",\"model\":\"espNRF\",\"sw_version\":\"");
	strcat(llwtd,AP_VER);
	if (wbuff[0]) {
	strcat(llwtd,"\",\"configuration_url\":\"http://");
	strcat(llwtd,wbuff);
	}
	strcat(llwtd,"\",\"manufacturer\":\"Espressif & Nordic Semiconductor\"},\"command_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/");
	strcat(llwtd,NAMONOF3);
	strcat(llwtd,"\",\"state_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/");
	strcat(llwtd,NAMONOF3);
	strcat(llwtd,"\",\"availability_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/status\"}");
	esp_mqtt_client_publish(mqttclient, llwtt, llwtd, 0, 1, 1);
//
	}
	if (NAMONOF4[0] && NAMONOF4[0] != 0x5f) {
	strcpy(llwtt,"homeassistant/switch/");
	strcat(llwtt,MQTT_BASE_TOPIC);
	strcat(llwtt,"/4x");
	strcat(llwtt,tESP8266Addr);
	strcat(llwtt,"/config");
	strcpy(llwtd,"{\"name\":\"");
//	strcat(llwtd,MQTT_BASE_TOPIC);
//	strcat(llwtd,".");
	strcat(llwtd,NAMONOF4);
	strcat(llwtd,"\",\"icon\":\"mdi:electric-switch\",\"uniq_id\":\"Switch_4.");
	strcat(llwtd,tESP8266Addr);
	strcat(llwtd,"\",\"device\":{\"identifiers\":[\"espNRF_");
	strcat(llwtd,tESP8266Addr);
	strcat(llwtd,"\"],\"name\":\"espNRF");
	if (espnrfnum)  {
	strcat(llwtd, ".");
	itoa(espnrfnum,buff,10);
	strcat(llwtd, buff);
	}
	strcat(llwtd,"\",\"model\":\"espNRF\",\"sw_version\":\"");
	strcat(llwtd,AP_VER);
	if (wbuff[0]) {
	strcat(llwtd,"\",\"configuration_url\":\"http://");
	strcat(llwtd,wbuff);
	}
	strcat(llwtd,"\",\"manufacturer\":\"Espressif & Nordic Semiconductor\"},\"command_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/");
	strcat(llwtd,NAMONOF4);
	strcat(llwtd,"\",\"state_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/");
	strcat(llwtd,NAMONOF4);
	strcat(llwtd,"\",\"availability_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/status\"}");
	esp_mqtt_client_publish(mqttclient, llwtt, llwtd, 0, 1, 1);
//
	}
	} else if (wmode == 2) {
	if (NAMONOF1[0] && NAMONOF1[0] != 0x5f) {
	strcpy(llwtt,"homeassistant/lock/");
	strcat(llwtt,MQTT_BASE_TOPIC);
	strcat(llwtt,"/1x");
	strcat(llwtt,tESP8266Addr);
	strcat(llwtt,"/config");
	strcpy(llwtd,"{\"name\":\"");
//	strcat(llwtd,MQTT_BASE_TOPIC);
//	strcat(llwtd,".");
	strcat(llwtd,NAMONOF1);
	strcat(llwtd,"\",\"icon\":\"mdi:lock\",\"uniq_id\":\"Lock_1.");
	strcat(llwtd,tESP8266Addr);
	strcat(llwtd,"\",\"device\":{\"identifiers\":[\"espNRF_");
	strcat(llwtd,tESP8266Addr);
	strcat(llwtd,"\"],\"name\":\"espNRF");
	if (espnrfnum)  {
	strcat(llwtd, ".");
	itoa(espnrfnum,buff,10);
	strcat(llwtd, buff);
	}
	strcat(llwtd,"\",\"model\":\"espNRF\",\"sw_version\":\"");
	strcat(llwtd,AP_VER);
	if (wbuff[0]) {
	strcat(llwtd,"\",\"configuration_url\":\"http://");
	strcat(llwtd,wbuff);
	}
	strcat(llwtd,"\",\"manufacturer\":\"Espressif & Nordic Semiconductor\"},\"command_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/");
	strcat(llwtd,NAMONOF1);
	strcat(llwtd,"/set\",\"state_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/");
	strcat(llwtd,NAMONOF1);
	strcat(llwtd,"\",\"availability_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/status\"}");
	esp_mqtt_client_publish(mqttclient, llwtt, llwtd, 0, 1, 1);
//
	strcpy(llwtt,"homeassistant/sensor/");
	strcat(llwtt,MQTT_BASE_TOPIC);
	strcat(llwtt,"/2x");
	strcat(llwtt,tESP8266Addr);
	strcat(llwtt,"/config");
	strcpy(llwtd,"{\"name\":\"");
//	strcat(llwtd,MQTT_BASE_TOPIC);
//	strcat(llwtd,".");
	strcat(llwtd,NAMONOF1);
	strcat(llwtd,"\",\"icon\":\"mdi:counter\",\"uniq_id\":\"Count_1.");
	strcat(llwtd,tESP8266Addr);
	strcat(llwtd,"\",\"device\":{\"identifiers\":[\"espNRF_");
	strcat(llwtd,tESP8266Addr);
	strcat(llwtd,"\"],\"name\":\"espNRF");
	if (espnrfnum)  {
	strcat(llwtd, ".");
	itoa(espnrfnum,buff,10);
	strcat(llwtd, buff);
	}
	strcat(llwtd,"\",\"model\":\"espNRF\",\"sw_version\":\"");
	strcat(llwtd,AP_VER);
	if (wbuff[0]) {
	strcat(llwtd,"\",\"configuration_url\":\"http://");
	strcat(llwtd,wbuff);
	}
	strcat(llwtd,"\",\"manufacturer\":\"Espressif & Nordic Semiconductor\"},\"state_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/");
	strcat(llwtd,NAMONOF1);
	strcat(llwtd,"/count\",\"availability_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/status\"}");
	esp_mqtt_client_publish(mqttclient, llwtt, llwtd, 0, 1, 1);
//


        }
	} else if (wmode == 3) {
	if (NAMONOF1[0] && NAMONOF1[0] != 0x5f) {
	strcpy(llwtt,"homeassistant/cover/");
	strcat(llwtt,MQTT_BASE_TOPIC);
	strcat(llwtt,"/1x");
	strcat(llwtt,tESP8266Addr);
	strcat(llwtt,"/config");
	strcpy(llwtd,"{\"name\":\"");
//	strcat(llwtd,MQTT_BASE_TOPIC);
//	strcat(llwtd,".");
	strcat(llwtd,NAMONOF1);
	strcat(llwtd,"\",\"icon\":\"mdi:garage\",\"uniq_id\":\"Cover_1.");
	strcat(llwtd,tESP8266Addr);
	strcat(llwtd,"\",\"device\":{\"identifiers\":[\"espNRF_");
	strcat(llwtd,tESP8266Addr);
	strcat(llwtd,"\"],\"name\":\"espNRF");
	if (espnrfnum)  {
	strcat(llwtd, ".");
	itoa(espnrfnum,buff,10);
	strcat(llwtd, buff);
	}
	strcat(llwtd,"\",\"model\":\"espNRF\",\"sw_version\":\"");
	strcat(llwtd,AP_VER);
	if (wbuff[0]) {
	strcat(llwtd,"\",\"configuration_url\":\"http://");
	strcat(llwtd,wbuff);
	}
	strcat(llwtd,"\",\"manufacturer\":\"Espressif & Nordic Semiconductor\"},\"command_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/");
	strcat(llwtd,NAMONOF1);
	strcat(llwtd,"/set\",\"set_position_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/");
	strcat(llwtd,NAMONOF1);
	strcat(llwtd,"/set\",\"position_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/");
	strcat(llwtd,NAMONOF1);
//	strcat(llwtd,"/position\",\"position_close\":0,\"position_open\":100,\"state_topic\":\"");
	strcat(llwtd,"/position\",\"state_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/");
	strcat(llwtd,NAMONOF1);
	strcat(llwtd,"/state\",\"availability_topic\":\"");
	strcat(llwtd,MQTT_BASE_TOPIC);
	strcat(llwtd,"/status\"}");
	esp_mqtt_client_publish(mqttclient, llwtt, llwtd, 0, 1, 1);
//
	}

	}


	}
	strcpy(ttopic,MQTT_BASE_TOPIC);
	strcat(ttopic,"/nrfcmd mqtt 1\r\n");
	if (floop) uart_write_bytes(UART_NUM_0, ttopic, strlen(ttopic));
	break;

	case MQTT_EVENT_DISCONNECTED:
	mqttConnected = false;
//	ESP_LOGI(TAG,"MQTT_EVENT_DISCONNECTED");
	strcpy(ttopic,MQTT_BASE_TOPIC);
	strcat(ttopic,"/nrfcmd mqtt 0\r\n");
	if (floop) uart_write_bytes(UART_NUM_0, ttopic, strlen(ttopic));
	break;

        case MQTT_EVENT_SUBSCRIBED:
//            ESP_LOGI(TAG,"MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
	break;
	case MQTT_EVENT_UNSUBSCRIBED:
//            ESP_LOGI(TAG,"MQTT_EVENT_UNSUBSCRIBED, msg_id=%d\n", event->msg_id);
	break;
	case MQTT_EVENT_PUBLISHED:
//            ESP_LOGI(TAG,"MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
	break;

	case MQTT_EVENT_DATA:
/*
	ESP_LOGI(TAG,"MQTT_EVENT_DATA");
	ESP_LOGI(TAG,"TOPIC=%.*s", event->topic_len, event->topic);
	ESP_LOGI(TAG,"DATA=%.*s", event->data_len, event->data);
*/
       	if (mqtdel && (event->topic_len) && (event->topic_len < 128)) {
	memset(ttopic,0,128);
        memcpy(ttopic, event->topic, event->topic_len);
        mqtdel = 20;
	if (event->data_len) esp_mqtt_client_publish(mqttclient, ttopic, "", 0, 1, 1);
	} else if((event->data_len) && (event->data_len < 64) && (event->topic_len) && (event->topic_len < 64)) {
	memset(ttopic,0,128);
        memcpy(ttopic, event->topic, event->topic_len);
//	ttopic[event->topic_len] = 0;
	strcpy(tbuff,MQTT_BASE_TOPIC);
	strcat(tbuff,"/");
	topoff = parsoff(event->topic,tbuff, event->topic_len);
	if ((wmode > 1) && NAMONOF1[0]) {
	strcat(tbuff,NAMONOF1);
	strcat(tbuff,"/");
	topoffcv = parsoff(event->topic,tbuff, event->topic_len);
	}
	if (topoff) {
	if (!memcmp(event->topic+topoff, "status", event->topic_len-topoff)) {
	} else if (!memcmp(event->topic+topoff, "rssi", event->topic_len-topoff)) {
	} else if (!memcmp(event->topic+topoff, "nrfrsp", event->topic_len-topoff)) {
	} else if ((wmode == 1) && NAMONOF1[0] && (!memcmp(event->topic+topoff, NAMONOF1, event->topic_len-topoff))) {
        t_tsonof = 2500;
	if ((!incascmp("1",event->data,event->data_len)) || (!incascmp("on",event->data,event->data_len))
		|| (!incascmp("true",event->data,event->data_len))) {
        if (!f_snfsnd1 || !(lvout ^ gpio_get_level(RELAY1))) {
	gpio_set_level(RELAY1, (lvout ^ 1));
        f_snfst1 = true;
        }
	} else if ((!incascmp("0",event->data,event->data_len)) || (!incascmp("off",event->data,event->data_len))
		|| (!incascmp("false",event->data,event->data_len))) {
        if (!f_snfsnd1 || (lvout ^ gpio_get_level(RELAY1))) {
	gpio_set_level(RELAY1, lvout);
        f_snfst1 = true;
        }
	}
        if (f_snfsnd1) f_snfsnd1 = false;
	else f_snfst1 = true;
	} else if ((wmode == 1) && NAMONOF2[0] && (!memcmp(event->topic+topoff, NAMONOF2, event->topic_len-topoff))) {
        t_tsonof = 2500;
	if ((!incascmp("1",event->data,event->data_len)) || (!incascmp("on",event->data,event->data_len))
		|| (!incascmp("true",event->data,event->data_len))) {
        if (!f_snfsnd2 || !(lvout ^ gpio_get_level(RELAY2))) {
	gpio_set_level(RELAY2, (lvout ^ 1));
        f_snfst2 = true;
        }
	} else if ((!incascmp("0",event->data,event->data_len)) || (!incascmp("off",event->data,event->data_len))
		|| (!incascmp("false",event->data,event->data_len))) {
        if (!f_snfsnd2 || (lvout ^ gpio_get_level(RELAY2))) {
	gpio_set_level(RELAY2, lvout);
        f_snfst2 = true;
        }
	}
        if (f_snfsnd2) f_snfsnd2 = false;
	else f_snfst2 = true;
	} else if ((wmode == 1) && NAMONOF3[0] && (!memcmp(event->topic+topoff, NAMONOF3, event->topic_len-topoff))) {
        t_tsonof = 2500;
	if ((!incascmp("1",event->data,event->data_len)) || (!incascmp("on",event->data,event->data_len))
		|| (!incascmp("true",event->data,event->data_len))) {
        if (!f_snfsnd3 || !(lvout ^ gpio_get_level(RELAY3))) {
	gpio_set_level(RELAY3, (lvout ^ 1));
        f_snfst3 = true;
        }
	} else if ((!incascmp("0",event->data,event->data_len)) || (!incascmp("off",event->data,event->data_len))
		|| (!incascmp("false",event->data,event->data_len))) {
        if (!f_snfsnd3 || (lvout ^ gpio_get_level(RELAY3))) {
	gpio_set_level(RELAY3, lvout);
        f_snfst3 = true;
        }
	}
        if (f_snfsnd3) f_snfsnd3 = false;
	else f_snfst3 = true;
	} else if ((wmode == 1) && NAMONOF4[0] && (!memcmp(event->topic+topoff, NAMONOF4, event->topic_len-topoff))) {
        t_tsonof = 2500;
	if ((!incascmp("1",event->data,event->data_len)) || (!incascmp("on",event->data,event->data_len))
		|| (!incascmp("true",event->data,event->data_len))) {
        if (!f_snfsnd4 || !(lvout ^ gpio_get_level(RELAY4))) {
	gpio_set_level(RELAY4, (lvout ^ 1));
        f_snfst4 = true;
        }
	} else if ((!incascmp("0",event->data,event->data_len)) || (!incascmp("off",event->data,event->data_len))
		|| (!incascmp("false",event->data,event->data_len))) {
        if (!f_snfsnd4 || (lvout ^ gpio_get_level(RELAY4))) {
	gpio_set_level(RELAY4, lvout);
        f_snfst4 = true;
        }
	}
        if (f_snfsnd4) f_snfsnd4 = false;
	else f_snfst4 = true;
	} else if ((wmode == 2) && topoffcv && (!memcmp(event->topic+topoffcv, "set", event->topic_len-topoffcv))) {
	esp_mqtt_client_publish(mqttclient, ttopic, "", 0, 1, 0);
	if (!incascmp("unlock",event->data,event->data_len)) {
        if (!f_snfsnd1 || !(lvout ^ gpio_get_level(RELAY1))) {
	gpio_set_level(RELAY1, (lvout ^ 1));
	tdelcover = 250;
        f_snfst1 = true;
        }
	} else if (!incascmp("lock",event->data,event->data_len)) {
        if (!f_snfsnd1 || (lvout ^ gpio_get_level(RELAY1))) {
	gpio_set_level(RELAY1, lvout);
	tdelcover = 0;
        f_snfst1 = true;
        }

	} else {
	int tempsz = event->data_len;
	if  (tempsz > 60) tempsz = 60;
	memset(tbuff,0,64);
	mystrcpy(tbuff, event->data, tempsz);
	if ((tbuff[0] > 0x2f) && (tbuff[0] < 0x3a)) {
	lockcount = atoi(tbuff);
        f_snfst1 = true;
	}
	}

	} else if ((wmode == 3) && topoffcv && (!memcmp(event->topic+topoffcv, "set", event->topic_len-topoffcv))) {
	esp_mqtt_client_publish(mqttclient, ttopic, "", 0, 1, 0);
	if ((!incascmp("open",event->data,event->data_len))) {
	stcvpos = 100;
	prcvpos = 255;

	} else if ((!incascmp("close",event->data,event->data_len))) {
	stcvpos = 0;
	prcvpos = 255;

	} else if ((!incascmp("stop",event->data,event->data_len))) {
	stcvpos = 254;
	prcvpos = 255;

	} else {
	int tempsz = event->data_len;
	memset(tbuff,0,64);
	if  (tempsz > 10) tempsz = 10;
	mystrcpy(tbuff, event->data, tempsz);
	uint8_t tcvpos = 255;
	if ((tbuff[0] > 0x30) && (tbuff[0] < 0x3a)) stcvpos = atoi(tbuff);
	if (tcvpos < 101) {
	if (tcvpos > 90) {
	stcvpos = 100;
	prcvpos = 255;
	} else if (tcvpos < 10) {
	stcvpos = 0;
	prcvpos = 255;
	} else  prcvpos = 255;
	} 

	}

        } else if ((event->topic_len + event->data_len) < 124) {
        memcpy(tbuff, event->data, event->data_len);
	tbuff[event->data_len] = 0;
	strcat (ttopic," ");	
	strcat (ttopic,tbuff);	
	strcat (ttopic,"\r\n");	
        if (floop) uart_write_bytes(UART_NUM_0, ttopic, strlen(ttopic));

	}
	}
	}
	break;

	case MQTT_EVENT_ERROR:
//	ESP_LOGI(TAG,"MQTT_EVENT_ERROR");
	break;
	default:
//	ESP_LOGI(TAG,"Other event id:%d", event->event_id);
	break;
	}
    return ESP_OK;
}


/*
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
//    ESP_LOGI(TAG,"Event dispatched from event loop base=%s, event_id=%d\n", base, event_id);
    mqtt_event_handler_cb(event_data);
}
*/
static void mqtt_app_start(void)
{
	char buff[16];
	char luri[128];
	char llwtt[16];
	strcpy(luri,"mqtt://");
	strcat(luri,MQTT_USER);
	strcat(luri,":");
	strcat(luri,MQTT_PASSWORD);
	strcat(luri,"@");
	strcat(luri,MQTT_SERVER);
	strcat(luri,":");
	itoa(mqtt_port,buff,10);
	strcat(luri,buff);
	strcpy(llwtt,MQTT_BASE_TOPIC);
	strcat(llwtt,"/status");

	esp_mqtt_client_config_t mqtt_cfg = {
	.uri = luri,
	.lwt_topic = llwtt,
	.lwt_msg = "offline",
	.lwt_retain = 1,
//	.lwt_qos = 1,
	.keepalive = 60,
	.client_id = MQTT_CLIENT_NAME,
	.buffer_size = 2048,
        .event_handle = mqtt_event_handler,
	};
	mqttConnected =false;
	mqttclient = esp_mqtt_client_init(&mqtt_cfg);
//	esp_mqtt_client_register_event(mqttclient, ESP_EVENT_ANY_ID, mqtt_event_handler, mqttclient);
	esp_mqtt_client_start(mqttclient);
}

//******************* WiFi **********************
static EventGroupHandle_t wifi_event_group;

static int s_retry_num = 0;

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    /* For accessing reason codes in case of disconnection */
    system_event_info_t *info = &event->event_info;
    switch (event->event_id) {
	case SYSTEM_EVENT_STA_START:
	tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA,MQTT_CLIENT_NAME);
		esp_wifi_connect();
		break;


	case SYSTEM_EVENT_STA_GOT_IP:
		xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
		s_retry_num = 0;
		NumWfConn++;
		if (!NumWfConn) NumWfConn--;
	        if (floop && MQTT_SERVER[0]) {
		char ttopic[32];
		if (floop && MQTT_SERVER[0]) esp_mqtt_client_reconnect(mqttclient);
//		if (floop && MQTT_SERVER[0]) esp_mqtt_client_start(mqttclient);
		strcpy(ttopic,MQTT_BASE_TOPIC);
		strcat(ttopic,"/nrfcmd mqtt 1\r\n");
		uart_write_bytes(UART_NUM_0, ttopic, strlen(ttopic));
		}
            break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
//            ESP_LOGE(TAG, "Disconnect reason : %d", info->disconnected.reason);
            if (info->disconnected.reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT) {
                /*Switch to 802.11 bgn mode */
                esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCAL_11B | WIFI_PROTOCAL_11G | WIFI_PROTOCAL_11N);
            }
//	if (floop && MQTT_SERVER[0]) esp_mqtt_client_stop(mqttclient);
	tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA,MQTT_CLIENT_NAME);
		if ((wmode == 3) && NAMONOF1[0]) { 
		if ((lvout ^ gpio_get_level(RELAY1)) || (lvout ^ gpio_get_level(RELAY2))) s_retry_num = 0;
		}
	if (s_retry_num < WIFI_MAXIMUM_RETRY) {
		esp_wifi_connect();
		s_retry_num++;
		xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
	} else {
	xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
	}
		break;
	default:
		break;
    }
    return ESP_OK;
}





void wifi_init_sta(void)
{
	s_retry_num = 0;
	tcpip_adapter_init();
	wifi_event_group = xEventGroupCreate();
	esp_event_loop_init(wifi_event_handler, NULL);
//	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));


	wifi_config_t wifi_config = {
        .sta = {
		.threshold.authmode = WIFI_AUTH_WPA_PSK
        },
    };

	memcpy(wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
	memcpy(wifi_config.sta.password, WIFI_PASSWORD, sizeof(wifi_config.sta.password));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
	ESP_ERROR_CHECK(esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N));
	ESP_ERROR_CHECK(esp_wifi_start() );
	EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, false, false, portMAX_DELAY);
	if (bits & WIFI_FAIL_BIT) {
	ESP_ERROR_CHECK(esp_wifi_stop() );
	char DEFWFSSID[33];
	char DEFWFPSW[65];
	strcpy(DEFWFSSID,"espnrf");
	strcpy(DEFWFPSW,"12345678");
	memcpy(wifi_config.sta.ssid, DEFWFSSID, sizeof(wifi_config.sta.ssid));
	memcpy(wifi_config.sta.password, DEFWFPSW, sizeof(wifi_config.sta.password));
	bits = xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
	ESP_ERROR_CHECK(esp_wifi_start() );
	bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, false, false, portMAX_DELAY);
	if (bits & WIFI_FAIL_BIT) {
	esp_restart();
	}
	}
}


//******************* uart **********************
bool rxlnread()
{
	bool done = false;
	bool ready = false;
	uint8_t a;
	while (!done) {
	if (cptbufrx >= lenbufrx) {
	uint rx_size;	
	uart_get_buffered_data_len(UART_NUM_0, &rx_size);
	if (rx_size) {
        lenbufrx = uart_read_bytes(UART_NUM_0, bufferrx, BUFFER_SIZE, 0 / portTICK_RATE_MS);
	cptbufrx = 0;
	if (lenbufrx < 0) lenbufrx = 0;
	if (!lenbufrx) done = true;
	} else done = true;
	}
	if (!done) {
	if (cptbufln >= (BUFFER_SIZE-1)) cptbufln = 0;
	a = bufferrx[cptbufrx];
	bufferln[cptbufln] = a;
	cptbufrx++;	
	cptbufln++;
	bufferln[cptbufln] = 0;
	if ((cptbufln == 1) && (a != 'n')) cptbufln = 0;
	if ((cptbufln == 2) && (a != 'r')) cptbufln = 0;
	if ((cptbufln == 3) && (a != 'f')) cptbufln = 0;
	if (a > 0x7f) cptbufln = 0;
	if ((a != 10) && (a != 13) && (a < 0x20)) cptbufln = 0;
	if (cptbufln == 0) bufferln[0] = 0;
	if (a == 10) {
	bufferln[cptbufln] = 0;
	cptbufln = 0;
	done = true;
	ready = true;
	}
	}
	}
	return ready;
}

void rxlnparse()
{
	MQTT_DATA[0] = 0;
	MQTT_TOPIC[0] = 0;
	if(strlen(bufferln) < 8) return;
	strcpy(bufferln1,bufferln); 
	char MQTT_PRFX[8];
	char buff[8];
	int i = 0; // read nrfx/
	int j = 0;
		while(i < 5) {
		MQTT_PRFX[j] = bufferln[i];
		i++; j++;
		}
        MQTT_PRFX[j] = 0;
        strcpy(buff,MQTT_BASE_TOPIC);
	strcat(buff,"/");
        //check prefix "nrf1/"
        if (strcmp(MQTT_PRFX,buff) && strcmp(MQTT_PRFX,"nrfx/")) return;
	j = 0;
		while(i < 8) {
		MQTT_PRFX[j] = bufferln[i];
		i++; j++;
		}
	MQTT_PRFX[j] = 0;
        // check if next 16 bytes of fw stored in NRF24LE1 memory
	if (!strcmp(MQTT_PRFX,":Ok")) {
	f_tfwnrf = 0; f_rfwnrf = true; return;
	}
	if (!strcmp(MQTT_PRFX,":Er")) {
	f_tfwnrf = 0; f_lfwnrf = false; f_rfwnrf = true;
	f_ffwnrf = true; f_sfwnrf = true; return;
          }
        // get rx topic name (begins at char 5)
		strcpy(MQTT_TOPIC,buff);
		i = 5; // ignore nrf1/
		j = strlen(MQTT_TOPIC);
		while ((bufferln[i] != ' ') && (i < BUFFER_SIZE) && (j < (MAX_TOPIC_LEN - 8))) {
		MQTT_TOPIC[j] = bufferln[i];
		i++; j++;
	        }
	        MQTT_TOPIC[j] = 0;
	        if ( j > 37 ) {
		MQTT_TOPIC[0] = 0;
		return;
		}
        // get rx topic data
		j=0;
                i++; // Skip ;
                while(bufferln[i] > 31 && i < BUFFER_SIZE && j < (BUFFER_SIZE-1)) {
		MQTT_DATA[j] = bufferln[i];
		i++; j++;
                }
		MQTT_DATA[j] = 0;
		if ( j > 32 ) {
		MQTT_TOPIC[0] = 0;
		MQTT_DATA[0] = 0;
		}
}

bool setsmqtt(uint8_t mqpar)
{
	bool result = false;
	char tbuft[32];
	char tbufr[16];
	uint8_t timeout;
	uint8_t fretr = 5;
	while (fretr && !result) {
	while (rxlnread())  vTaskDelay(100 / portTICK_PERIOD_MS);
	strcpy(tbuft,MQTT_BASE_TOPIC);
	strcat(tbuft,"/nrfcmd mqtt ");
        (mqpar)? strcat(tbuft,"1\r\n") : strcat(tbuft,"0\r\n");
	uart_write_bytes(UART_NUM_0, tbuft, strlen(tbuft));
	strcpy(tbufr,MQTT_BASE_TOPIC);
	strcat(tbufr,"/nrfrsp");
	timeout = 5;
	while (--timeout) {
	if (rxlnread()) {
	rxlnparse();
	if ((!strcmp(MQTT_TOPIC,tbufr)) && (!strcmp(MQTT_DATA,"Ok"))) {
	timeout = 1;
	result = true;
	} else {
	timeout = 1;
	}
	} else vTaskDelay(100 / portTICK_PERIOD_MS);
	}
	fretr--;
	}
	return result;
}


bool setstha()
{
	bool result = false;
	char tbuft[32];
	char tbufr[16];
	uint8_t timeout;
	uint8_t fretr = 5;
	while (fretr && !result) {
	while (rxlnread())  vTaskDelay(100 / portTICK_PERIOD_MS);
	strcpy(tbuft,MQTT_BASE_TOPIC);
	strcat(tbuft,"/nrfcmd stha ");
        (FDHass)? strcat(tbuft,"1\r\n") : strcat(tbuft,"0\r\n");
	uart_write_bytes(UART_NUM_0, tbuft, strlen(tbuft));
	strcpy(tbufr,MQTT_BASE_TOPIC);
	strcat(tbufr,"/nrfrsp");
	timeout = 5;
	while (--timeout) {
	if (rxlnread()) {
	rxlnparse();
	if ((!strcmp(MQTT_TOPIC,tbufr)) && (!strcmp(MQTT_DATA,"Ok"))) {
	timeout = 1;
	result = true;
	} else {
	timeout = 1;
	}
	} else vTaskDelay(100 / portTICK_PERIOD_MS);
	}
	fretr--;
	}
	return result;
}


bool hassdiscovery()
{
	bool result = false;
	char buft[64];
	char bufd[2048];
	char tbuft[32];
	char tbufr[16];
	char ttbuff[8];
	int  timeout;
	tcpip_adapter_ip_info_t ipInfo;
	char wbuff[256];
//2
	if (FDHass && tESP8266Addr[0] && mqttConnected) {
	memset(wbuff,0,32);
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
	sprintf(wbuff, "%d.%d.%d.%d", IP2STR(&ipInfo.ip));
//3
	uint8_t dev = 0;
	while (dev < 16) {
	dev++;
//4
	uint8_t par = 0;
	while (par < 4) {
	par++;
//5
	while (rxlnread())  vTaskDelay(100 / portTICK_PERIOD_MS);
	strcpy(tbuft,MQTT_BASE_TOPIC);
	strcat(tbuft,"/nrfcmd get ");
        itoa(dev,ttbuff,10);
	strcat(tbuft,ttbuff);
	strcat(tbuft," ");
        itoa(par,ttbuff,10);
	strcat(tbuft,ttbuff);
	strcat(tbuft,"\r\n");
        uart_write_bytes(UART_NUM_0, tbuft, strlen(tbuft));
	strcpy(tbufr,MQTT_BASE_TOPIC);
	strcat(tbufr,"/nrfrsp");
	timeout = 5;
	while (--timeout) {
	MQTT_DATA[0] = 0;
	MQTT_TOPIC[0] = 0;
	if (rxlnread()) {
	rxlnparse();
	if (!strcmp(MQTT_TOPIC,tbufr)) timeout = 1;
	else {
        MQTT_DATA[0] = 0;
	MQTT_TOPIC[0] = 0;
	vTaskDelay(100 / portTICK_PERIOD_MS);
	}
	} else vTaskDelay(100 / portTICK_PERIOD_MS);
	}

	if (MQTT_DATA[0] && MQTT_DATA[0] != 0x5f) {

	if (par < 3) {
	strcpy(buft,"homeassistant/switch/");
	strcat(buft,MQTT_BASE_TOPIC);
	strcat(buft,"/");
        itoa(dev,ttbuff,10);
	strcat(buft,ttbuff);
	strcat(buft,"x");
        itoa(par,ttbuff,10);
	strcat(buft,ttbuff);
	strcat(buft,"x");
	strcat(buft,tESP8266Addr);
	strcat(buft,"/config");
	strcpy(bufd,"{\"name\":\"");
//	strcat(bufd,MQTT_BASE_TOPIC);
//	strcat(bufd,".");
	strcat(bufd,MQTT_DATA);
	strcat(bufd,"\",\"icon\":\"mdi:electric-switch\",\"uniq_id\":\"Switch_");
        itoa(dev,ttbuff,10);
	strcat(bufd,ttbuff);
	strcat(bufd,".");
        itoa(par,ttbuff,10);
	strcat(bufd,ttbuff);
	strcat(bufd,".");
	strcat(bufd,tESP8266Addr);
	strcat(bufd,"\",\"device\":{\"identifiers\":[\"NRF24_");
        itoa(dev,ttbuff,10);
	strcat(bufd,ttbuff);
	strcat(bufd,".");
	strcat(bufd,tESP8266Addr);
	strcat(bufd,"\"],\"name\":\"");
	strcat(bufd,MQTT_BASE_TOPIC);
	strcat(bufd,".");
        itoa(dev,ttbuff,10);
	if (dev < 10) strcat(bufd,"0");
	strcat(bufd,ttbuff);
	strcat(bufd,"\",\"model\":\"NRF24LE1\",\"via_device\":\"espNRF_");
	strcat(bufd,tESP8266Addr);
	strcat(bufd,"\",\"manufacturer\":\"Nordic Semiconductor\"},\"command_topic\":\"");
	strcat(bufd,MQTT_BASE_TOPIC);
	strcat(bufd,"/");
	strcat(bufd,MQTT_DATA);
	strcat(bufd,"\",\"state_topic\":\"");
	strcat(bufd,MQTT_BASE_TOPIC);
	strcat(bufd,"/");
	strcat(bufd,MQTT_DATA);
	strcat(bufd,"\",\"availability_topic\":\"");
	strcat(bufd,MQTT_BASE_TOPIC);
	strcat(bufd,"/status");
	if (dev < 10) strcat(bufd,"0");
        itoa(dev,ttbuff,10);
	strcat(bufd,ttbuff);
	strcat(bufd,"\"}");
	esp_mqtt_client_publish(mqttclient, buft, bufd, 0, 1, 1);
//
	} else if (par == 3) {
	strcpy(buft,"homeassistant/sensor/");
	strcat(buft,MQTT_BASE_TOPIC);
	strcat(buft,"/");
        itoa(dev,ttbuff,10);
	strcat(buft,ttbuff);
	strcat(buft,"x");
        itoa(par,ttbuff,10);
	strcat(buft,ttbuff);
	strcat(buft,"x");
       	strcat(buft,tESP8266Addr);
	strcat(buft,"/config");
	strcpy(bufd,"{\"name\":\"");
//	strcat(bufd,MQTT_BASE_TOPIC);
//	strcat(bufd,".");
	strcat(bufd,MQTT_DATA);
	strcat(bufd,"\",\"icon\":\"mdi:thermometer\",\"uniq_id\":\"Temp_");
        itoa(dev,ttbuff,10);
	strcat(bufd,ttbuff);
	strcat(bufd,".");
        itoa(par,ttbuff,10);
	strcat(bufd,ttbuff);
	strcat(bufd,".");
	strcat(bufd,tESP8266Addr);
	strcat(bufd,"\",\"device\":{\"identifiers\":[\"NRF24_");
        itoa(dev,ttbuff,10);
	strcat(bufd,ttbuff);
	strcat(bufd,".");
	strcat(bufd,tESP8266Addr);
	strcat(bufd,"\"],\"name\":\"");
	strcat(bufd,MQTT_BASE_TOPIC);
	strcat(bufd,".");
        itoa(dev,ttbuff,10);
	if (dev < 10) strcat(bufd,"0");
	strcat(bufd,ttbuff);
	strcat(bufd,"\",\"model\":\"NRF24LE1\",\"via_device\":\"espNRF_");
	strcat(bufd,tESP8266Addr);
	strcat(bufd,"\",\"manufacturer\":\"Nordic Semiconductor\"},\"device_class\":\"temperature\",\"state_class\":\"measurement\",\"state_topic\":\"");
	strcat(bufd,MQTT_BASE_TOPIC);
	strcat(bufd,"/");
	strcat(bufd,MQTT_DATA);
	strcat(bufd,"\",\"unit_of_meas\":\"\xc2\xb0\x43\",\"availability_topic\":\"");
	strcat(bufd,MQTT_BASE_TOPIC);
	strcat(bufd,"/status");
	if (dev < 10) strcat(bufd,"0");
        itoa(dev,ttbuff,10);
	strcat(bufd,ttbuff);
	strcat(bufd,"\"}");
	esp_mqtt_client_publish(mqttclient, buft, bufd, 0, 1, 1);
//
	} else if (par == 4) {
	strcpy(buft,"homeassistant/sensor/");
	strcat(buft,MQTT_BASE_TOPIC);
	strcat(buft,"/");
        itoa(dev,ttbuff,10);
	strcat(buft,ttbuff);
	strcat(buft,"x");
        itoa(par,ttbuff,10);
	strcat(buft,ttbuff);
	strcat(buft,"x");
	strcat(buft,tESP8266Addr);
	strcat(buft,"/config");
	strcpy(bufd,"{\"name\":\"");
//	strcat(bufd,MQTT_BASE_TOPIC);
//	strcat(bufd,".");
	strcat(bufd,MQTT_DATA);
	strcat(bufd,"\",\"icon\":\"mdi:alpha-v-circle-outline\",\"uniq_id\":\"Volt_");
        itoa(dev,ttbuff,10);
	strcat(bufd,ttbuff);
	strcat(bufd,".");
        itoa(par,ttbuff,10);
	strcat(bufd,ttbuff);
	strcat(bufd,".");
	strcat(bufd,tESP8266Addr);
	strcat(bufd,"\",\"device\":{\"identifiers\":[\"NRF24_");
        itoa(dev,ttbuff,10);
	strcat(bufd,ttbuff);
	strcat(bufd,".");
	strcat(bufd,tESP8266Addr);
	strcat(bufd,"\"],\"name\":\"");
	strcat(bufd,MQTT_BASE_TOPIC);
	strcat(bufd,".");
        itoa(dev,ttbuff,10);
	if (dev < 10) strcat(bufd,"0");
	strcat(bufd,ttbuff);
	strcat(bufd,"\",\"model\":\"NRF24LE1\",\"via_device\":\"espNRF_");
	strcat(bufd,tESP8266Addr);
	strcat(bufd,"\",\"manufacturer\":\"Nordic Semiconductor\"},\"device_class\":\"voltage\",\"state_class\":\"measurement\",\"state_topic\":\"");
	strcat(bufd,MQTT_BASE_TOPIC);
	strcat(bufd,"/");
	strcat(bufd,MQTT_DATA);
	strcat(bufd,"\",\"unit_of_meas\":\"V\",\"availability_topic\":\"");
	strcat(bufd,MQTT_BASE_TOPIC);
	strcat(bufd,"/status");
	if (dev < 10) strcat(bufd,"0");
        itoa(dev,ttbuff,10);
	strcat(bufd,ttbuff);
	strcat(bufd,"\"}");
	esp_mqtt_client_publish(mqttclient, buft, bufd, 0, 1, 1);
//
	}
//5
	}
//4
        }
//3
	}
//2
	}
	return result;
}


//*************** http server *******************

/* HTTP GET main handler */
static esp_err_t pmain_get_handler(httpd_req_t *req)
{
	char bufip[32] = {0};
	time_t now;
	char strftime_buf[64];
	struct tm timeinfo;
	time(&now);
	localtime_r(&now, &timeinfo);
	strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
	int buf_len;
	buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
	if (buf_len > 1) {
	if (buf_len >31) buf_len = 31;
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", bufip, buf_len) == ESP_OK) {
//ESP_LOGI(TAG,"Ip from header %s", bufip); 
        }
    }
//	char bsend[14200];
	char *bsend = NULL;
	bsend = malloc(14200);
	if (bsend == NULL) {
	MemErr++;
	if (!MemErr) MemErr--;
//
	} else {	
	int FreeMem = esp_get_free_heap_size();
        char buff[64];
	strcpy(bsend,"<!DOCTYPE html><html>");
	strcat(bsend,"<head><title>espNRF</title>");
        strcat(bsend,"<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
        strcat(bsend,cssDatasheet);
	strcat(bsend,"</head><body class='bodymenu'><header class='headermenu'><h1>espNRF");
	itoa(espnrfnum,buff,10);
	strcat(bsend, buff);
	strcat(bsend,"</h1>");
	strcat(bsend,"<div class='menubar'><a class='menu active' href='.'>&#8962;<span class='showmenulabel'>Main");
//	strcat(bsend,"</span></a><a class='menu' href='livedata'>&#10740;<span class='showmenulabel'>Live Data");
	strcat(bsend,"</span></a><a class='menu' href='resetnrf'>&#128259;<span class='showmenulabel'>Reset NRF");
	strcat(bsend,"</span></a><a class='menu' href='resetesp'>&#128259;<span class='showmenulabel'>Reset ESP");
	strcat(bsend,"</span></a><a class='menu' href='loadnrf'>&#10548;<span class='showmenulabel'>Load NRF Firmware");
	strcat(bsend,"</span></a><a class='menu' href='loadesp'>&#10548;<span class='showmenulabel'>Load ESP Firmware");
	strcat(bsend,"</span></a><a class='menu' href='setting'>&#9881;<span class='showmenulabel'>Setting</span></a></div></header>");
      // Live data
	strcat(bsend,"<h3>NRF Live Data *</h3>");
	strcat(bsend,"<input type=\"button\" value =\"Pause\" onclick=\"stopUpdate();\" />");                                                            // Pause
	strcat(bsend,"<input type=\"button\" value =\"Restart\" onclick=\"restartUpdate();\" />");                                                       // Restart
	strcat(bsend,"<input type=\"button\" value =\"Refresh\" onclick=\"window.location.reload(true);\" />");                                          // Refresh
	strcat(bsend,"<input type=\"text\" id=\"mySearch\" onkeyup=\"filterLines()\" placeholder=\"Search for...\" title=\"Type in a name\"><br />");    // Search
	strcat(bsend,"<table id=\"liveData\" class='multirow';>");                                                                                       // Table of x lines
	strcat(bsend,"<tr class=\"header\"><th style='text-align:left;'>Raw Data</th><th style='text-align:left;'> MQTT Topic </th><th style='text-align:left;'> MQTT Data </th></tr>");

	for (int i = 0; i < (5); i++) { // not too high to avoid overflow
	strcat(bsend,"<tr id=\"data");
	itoa(i,buff,10);
	strcat(bsend, buff);
	strcat(bsend,"\"><td></td><td></td><td></td></tr>");
	}
        strcat(bsend,"</table>");
        strcat(bsend,"<script>function filterLines() {");  // Script to filter lines
        strcat(bsend," var input, filter, table, tr, td, i;");
        strcat(bsend," input = document.getElementById(\"mySearch\");");
        strcat(bsend," filter = input.value.toUpperCase();");
        strcat(bsend," table = document.getElementById(\"liveData\");");
        strcat(bsend," tr = table.getElementsByTagName(\"tr\");");
        strcat(bsend," for (i = 0; i < tr.length; i++) {");
        strcat(bsend,"  td = tr[i].getElementsByTagName(\"td\")[0];");
        strcat(bsend,"  if (td) { if (td.innerHTML.toUpperCase().indexOf(filter) > -1) {");
        strcat(bsend,"  tr[i].style.display = \"\"; } else {  tr[i].style.display = \"none\";"); 
        strcat(bsend," }}}}</script><script>"); // Script to update data and move to next line
        strcat(bsend,"var x = setInterval(function() {loadData(\"data.txt\",updateData)}, 500);");
        strcat(bsend,"function loadData(url, callback){ var xhttp = new XMLHttpRequest();");
        strcat(bsend,"xhttp.onreadystatechange = function(){ if(this.readyState == 4 && this.status == 200){");
        strcat(bsend," callback.apply(xhttp);}};xhttp.open(\"GET\", url, true);xhttp.send();}");
        strcat(bsend,"var memorized_data; function updateData(){ if (memorized_data != this.responseText) {");
	for (int i = (5 - 1); i > 0; i--) {	// not too high to avoid overflow
        strcat(bsend,"document.getElementById(\"data");
	itoa(i,buff,10);
	strcat(bsend, buff);
        strcat(bsend,"\").innerHTML = document.getElementById(\"data");
	i--;
	itoa(i,buff,10);
	strcat(bsend, buff);
	i++;
        strcat(bsend,"\").innerHTML;");
	}
        strcat(bsend,"} document.getElementById(\"data0\").innerHTML = this.responseText;");
        strcat(bsend," memorized_data = this.responseText;"); // memorize new data
        strcat(bsend,"filterLines();}"); // apply filter from mySearch input
        strcat(bsend,"function stopUpdate(){ clearInterval(x);} function restartUpdate(){");
        strcat(bsend," x = setInterval(function() {loadData(\"data.txt\",updateData)}, 500);}</script>");
        strcat(bsend,"<div class='note'>* see Live \"Data tab\" for more lines - web view may not catch all frames, MQTT debug is more accurate</div>");
		// Commands to RFLink
        strcat(bsend,"<h3>Commands to NRF</h3><br />");
        strcat(bsend,"<form action=\"/webcmd\" id=\"form_command\" style=\"float: left;\"><input type=\"text\" id=\"command\" name=\"command\">");
        strcat(bsend,"<input type=\"submit\" value=\"Send\"><a class='button help' href='https://github.com/alutov/nrf24le1-espnrf_gateway_and_remote_switch_for_livolo_etc' target='_blank'>&#10068;</a></form>");
	for (int i = 0; i < (sizeof(USER_CMDs) / sizeof(USER_CMDs[0])); i++) {     // User commands defined in USER_CMDs for quick access
        strcat(bsend,"<a class='button link' style=\"float: left;\" href=\"javascript:{}\"");
        strcat(bsend,"onclick=\"document.getElementById('command').value = '");
        strcat(bsend,USER_CMDs[i].command);
        strcat(bsend,"'; return false;\">");
        strcat(bsend,USER_CMDs[i].text);
        strcat(bsend,"</a>");
	}
        strcat(bsend,"<br style=\"clear: both;\"/>");
		// System Info
        strcat(bsend,"<h3>System Info</h3><table class='normal'><tr><td style='min-width:150px;'>Version</td><td style='width:80%;'>");
        strcat(bsend,AP_VER);
/*
		strcat(bsend,"</td></tr><tr><td>ESP-IDF version</td><td>");
		strcat(bsend,IDF_VER);
*/
		strcat(bsend,"</td></tr><tr><td>Local time and date</td><td>");
		strcat(bsend,strftime_buf);
	if (wmode == 1) {	
	strcat(bsend,"</td></tr><tr><td>Button 1-2-3-4/Relay 1-2-3-4</td><td>");
	strcpy(buff,"x");
	if (NAMONOF1[0]) itoa(gpio_get_level(BUTTON1),buff,10);
	strcat(bsend, buff);
	strcat(bsend, "-");
	strcpy(buff,"x");
	if (NAMONOF2[0]) itoa(gpio_get_level(BUTTON2),buff,10);
	strcat(bsend, buff);
	strcat(bsend, "-");
	strcpy(buff,"x");
	if (NAMONOF3[0]) itoa(gpio_get_level(BUTTON3),buff,10);
	strcat(bsend, buff);
	strcat(bsend, "-");
	strcpy(buff,"x");
	if (NAMONOF4[0]) itoa(gpio_get_level(BUTTON4),buff,10);
	strcat(bsend, buff);
	strcat(bsend, "/");
	strcpy(buff,"x");
	if (NAMONOF1[0]) itoa(gpio_get_level(RELAY1),buff,10);
	strcat(bsend, buff);
	strcat(bsend, "-");
	strcpy(buff,"x");
	if (NAMONOF2[0]) itoa(gpio_get_level(RELAY2),buff,10);
	strcat(bsend, buff);
	strcat(bsend, "-");
	strcpy(buff,"x");
	if (NAMONOF3[0]) itoa(gpio_get_level(RELAY3),buff,10);
	strcat(bsend, buff);
	strcat(bsend, "-");
	strcpy(buff,"x");
	if (NAMONOF4[0]) itoa(gpio_get_level(RELAY4),buff,10);
	strcat(bsend, buff);

	} else if (wmode == 2) {
	strcat(bsend,"</td></tr><tr><td>Sensor 1/Relay 1</td><td>");
	strcpy(buff,"x");
	if (NAMONOF1[0]) itoa(gpio_get_level(BUTTON1),buff,10);
	strcat(bsend, buff);
	strcat(bsend, "/");
	strcpy(buff,"x");
	if (NAMONOF1[0]) itoa(gpio_get_level(RELAY1),buff,10);
	strcat(bsend, buff);

	} else if (wmode == 3) {
//	strcat(bsend,"</td></tr><tr><td>Button 1-2/Sensor 1-2/Relay 1-2</td><td>");
	strcat(bsend,"</td></tr><tr><td>Button 1-2/Relay 1-2</td><td>");
	strcpy(buff,"x");
	if (NAMONOF1[0]) itoa(gpio_get_level(BUTTON1),buff,10);
	strcat(bsend, buff);
	strcat(bsend, "-");
	strcpy(buff,"x");
	if (NAMONOF1[0]) itoa(gpio_get_level(BUTTON2),buff,10);
	strcat(bsend, buff);
	strcat(bsend, "/");
/*
	strcpy(buff,"x");
	if (NAMONOF3[0]) itoa(gpio_get_level(BUTTON3),buff,10);
	strcat(bsend, buff);
	strcat(bsend, "-");
	strcpy(buff,"x");
	if (NAMONOF4[0]) itoa(gpio_get_level(BUTTON4),buff,10);
	strcat(bsend, buff);
	strcat(bsend, "/");
*/
	strcpy(buff,"x");
	if (NAMONOF1[0]) itoa(gpio_get_level(RELAY1),buff,10);
	strcat(bsend, buff);
	strcat(bsend, "-");
	strcpy(buff,"x");
	if (NAMONOF1[0]) itoa(gpio_get_level(RELAY2),buff,10);
	strcat(bsend, buff);

	}
        strcat(bsend,"</td></tr><tr><td>Uptime</td><td>");
	uptime_string_exp(buff);
	strcat(bsend,buff);
        strcat(bsend,"</td></tr><tr><td>Free memory</td><td>");
	itoa(FreeMem,buff,10);
	strcat(bsend,buff);
        strcat(bsend," bytes</td></tr>");
	wifi_ap_record_t wifidata;
	if (esp_wifi_sta_get_ap_info(&wifidata)==0){
        strcat(bsend,"<tr><td>WiFi network</td><td>");
        buff[31] = 0;
	memcpy(buff,wifidata.ssid,31);
        strcat(bsend,buff);
	strcat(bsend,"</td></tr><tr><td>WiFi connection count / RSSI</td><td>");
	itoa(NumWfConn,buff,10);
	strcat(bsend,buff);
	strcat(bsend," / ");
        itoa(wifidata.rssi,buff,10);
	strcat(bsend,buff);
        strcat(bsend," dB</td></tr><tr><td>WiFi IP address</td><td>");
        strcat(bsend,bufip);
	strcat(bsend,"</td></tr>");
	}
	strcat(bsend,"<tr><td>MQTT server:port</td><td>");
        if (MQTT_SERVER[0]) strcat(bsend,MQTT_SERVER);
	strcat(bsend,":");
	itoa(mqtt_port,buff,10);
	strcat(bsend,buff);
	strcat(bsend,"<tr><td>MQTT connection count / state</td><td>");
	itoa(NumMqConn,buff,10);
	strcat(bsend,buff);
	strcat(bsend," / ");
        (mqttConnected)? strcat(bsend,"Connected") : strcat(bsend,"Disconnected");

	strcat(bsend,"</td></tr></table><br><span style=\"font-size: 0.9em\">Running for ");
	uptime_string_exp(buff);
	strcat(bsend,buff);
        strcat(bsend," </span><input type=\"button\" value =\"Refresh\" onclick=\"window.location.reload(true);\" />");
	strcat(bsend,"<br><footer><h6>More info on <a href='https://github.com/alutov/nrf24le1-espnrf_gateway_and_remote_switch_for_livolo_etc' style='font-size: 15px; text-decoration: none'>github.com/alutov </a></h6>");

//itoa(strlen(bsend),buff,10);
//strcat(bsend,buff);
	httpd_resp_send(req, bsend, strlen(bsend));


	free(bsend);
	}
    return ESP_OK;
}

static const httpd_uri_t pmain = {
	.uri       = "/",
	.method    = HTTP_GET,
	.handler   = pmain_get_handler,
	.user_ctx  = NULL
};


/* HTTP GET webcmd handler */
static esp_err_t pwebcmd_get_handler(httpd_req_t *req)
{
//save command from header
	char bufcom[64];
	bufcom[0] = 0;
	uint8_t i = 0;
	uint8_t j = 0;
	uint8_t a;
	uint8_t b;
	uint8_t c;
	char*  buf;
	size_t buf_len;
	buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
	strcpy(bufcom,MQTT_BASE_TOPIC);
	strcat(bufcom,"/nrfcmd ");
	j = strlen(bufcom);
	i = parsoff(buf,"command=", buf_len);
	while (i && (i < buf_len)) {
	a = buf[i];
	i++;
	if (a == 0x25) {
	a = buf[i];
	b =  (a <= '9') ? (a - '0') : (a - 'A' + 10);
	b = (b & 0xf)*16;
	i++;
	a = buf[i];
	c =  (a <= '9') ? (a - '0') : (a - 'A' + 10);
	a = (c & 0xf) + b;
	i++;
	} else if (a == 0x2b) a = 0x20;   //+ to space

	if (a) bufcom[j] = a;
	else {
	bufcom[j] = 0x0d;
	j++;
	bufcom[j] = 0x0a;
//	i = 0;
	i = buf_len;
	}
	j++;
	bufcom[j] = 0;
	}	

//ESP_LOGI(TAG, "Found URL command => %s", bufcom);
        }
	}
        if (floop) uart_write_bytes(UART_NUM_0, bufcom, strlen(bufcom));
	httpd_resp_set_status(req, "303 See Other");
	httpd_resp_set_hdr(req, "Location", "/");
	httpd_resp_send(req, NULL, 0);  // Response body can be empty
    return ESP_OK;
}
static const httpd_uri_t pwebcmd = {
	.uri       = "/webcmd",
	.method    = HTTP_GET,
	.handler   = pwebcmd_get_handler,
	.user_ctx  = NULL
};


/* HTTP GET data.txt handler */
static esp_err_t pdatatxt_get_handler(httpd_req_t *req)
{
        char buff[32];
	char bsend[512];
	bsend[0] = 0;	
	if (f_sfwnrf)  {
        if (f_lfwnrf) {
	strcpy(bsend,"<td>Loading NRF in progress ... ");
	itoa(UPLIDX,buff,10);
	strcat(bsend, buff);
	strcat(bsend," bytes of ");
	itoa(UPLBUF_SIZE,buff,10);
	strcat(bsend, buff);
	strcat(bsend,"</td><td></td><td></td>");
	f_sfwnrf = false;
        } else if (f_ffwnrf) {
	strcpy(bsend,"<td>Loading NRF Failed</td><td></td><td></td>");
	f_sfwnrf = false;
        } else {
	strcpy(bsend,"<td>Loading NRF Ok</td><td></td><td></td>");
	f_sfwnrf = false;
        }
	} else {
	strcpy(bsend,"<td>");
	strcat(bsend,bufferln1);
	strcat(bsend,"</td><td>");
	strcat(bsend,MQTT_TOPIC);
	strcat(bsend,"</td><td>");
	strcat(bsend,MQTT_DATA);
	strcat(bsend,"</td>");
	}
	httpd_resp_send(req, bsend, strlen(bsend));
    return ESP_OK;
}
static const httpd_uri_t pdatatxt = {
	.uri       = "/data.txt",
	.method    = HTTP_GET,
	.handler   = pdatatxt_get_handler,
	.user_ctx  = NULL
};



/* HTTP GET reboot nrf handler */
static esp_err_t presetnrf_get_handler(httpd_req_t *req)
{
	char buf1[512] = {0};
//save ip from header
	char bufip[32] = {0};
	int buf_len;
	buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
	if (buf_len > 1) {
	if (buf_len >31) buf_len = 31;
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", bufip, buf_len) == ESP_OK) {
        }
	}
	gpio_set_level(NRF_RESET_PIN, 0);  // reset nrf24
	strcpy(buf1,"<!DOCTYPE html><html><head><title>espNRF</title><meta name='viewport' content='width=device-width, initial-scale=1.0'></head><body>Rebooting NRF...</body></html><meta http-equiv=\"refresh\" content=\"3;URL=http://");
	strcat(buf1, bufip);
	strcat(buf1,"/\"</body></html>");
	httpd_resp_send(req, buf1, strlen(buf1));
	vTaskDelay(500 / portTICK_PERIOD_MS);
	gpio_set_level(NRF_RESET_PIN, 1);  // start nrf24

    return ESP_OK;
}
static const httpd_uri_t presetnrf = {
	.uri       = "/resetnrf",
	.method    = HTTP_GET,
	.handler   = presetnrf_get_handler,
	.user_ctx  = NULL
};

/* HTTP GET reboot esp handler */
static esp_err_t presetesp_get_handler(httpd_req_t *req)
{
	char buf1[512] = {0};
//save ip from header
	char bufip[32] = {0};
	int buf_len;
	buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
	if (buf_len > 1) {
	if (buf_len >31) buf_len = 31;
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", bufip, buf_len) == ESP_OK) {
        }
	}
	strcpy(buf1,"<!DOCTYPE html><html><head><title>espNRF</title><meta name='viewport' content='width=device-width, initial-scale=1.0'></head><body>Rebooting esp...</body></html><meta http-equiv=\"refresh\" content=\"3;URL=http://");
        strcat(buf1, bufip);
	strcat(buf1,"/\"</body></html>");
	httpd_resp_send(req, buf1, strlen(buf1));

	vTaskDelay(1000 / portTICK_PERIOD_MS);
	esp_restart();
    return ESP_OK;
}
static const httpd_uri_t presetesp = {
	.uri       = "/resetesp",
	.method    = HTTP_GET,
	.handler   = presetesp_get_handler,
	.user_ctx  = NULL
};

/* HTTP GET setting handler */
static esp_err_t psetting_get_handler(httpd_req_t *req)
{
//	char bsend[13000];
	char *bsend = NULL;
	bsend = malloc(13000);
	if (bsend == NULL) {
	MemErr++;
	if (!MemErr) MemErr--;

	} else {	
        char buff[32];
	strcpy(bsend,"<!DOCTYPE html><html>");
	strcat(bsend,"<head><title>espNRF</title>");
        strcat(bsend,"<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
        strcat(bsend,cssDatasheet);
	strcat(bsend,"</head><body class='bodymenu'><header class='headermenu'><h1>espNRF");
	itoa(espnrfnum,buff,10);
	strcat(bsend, buff);
	strcat(bsend,"</h1>");
	strcat(bsend,"<div class='menubar'><a class='menu' href='.'>&#8962;<span class='showmenulabel'>Main");
//	strcat(bsend,"</span></a><a class='menu' href='livedata'>&#10740;<span class='showmenulabel'>Live Data");
	strcat(bsend,"</span></a><a class='menu' href='resetnrf'>&#128259;<span class='showmenulabel'>Reset NRF");
	strcat(bsend,"</span></a><a class='menu' href='resetesp'>&#128259;<span class='showmenulabel'>Reset ESP");
	strcat(bsend,"</span></a><a class='menu' href='loadnrf'>&#10548;<span class='showmenulabel'>Load NRF Firmware");
	strcat(bsend,"</span></a><a class='menu' href='loadesp'>&#10548;<span class='showmenulabel'>Load ESP Firmware");
	strcat(bsend,"</span></a><a class='menu active' href='setting'>&#9881;<span class='showmenulabel'>Setting</span></a></div>");
	strcat(bsend,"</header><table class='normal'><h3>Wifi Setting</h3><br/><body>");
	strcat(bsend,"<form method=\"POST\" action=\"/setsave\"><input name=\"swfid\" value=\"");
	if (WIFI_SSID[0]) strcat(bsend,WIFI_SSID);
	strcat(bsend,"\" size=\"15\">SSID &emsp;<input type=\"password\" input name=\"swfpsw\" value=\"");
	if (WIFI_PASSWORD[0]) strcat(bsend,WIFI_PASSWORD);
	strcat(bsend,"\"size=\"31\">Password</br><h3>MQTT Setting</h3><br/><input name=\"smqsrv\" value=\"");
	if (MQTT_SERVER[0]) strcat(bsend,MQTT_SERVER);
	strcat(bsend,"\"size=\"15\">Server &emsp;<input name=\"smqprt\" type=\"number\" value=\"");
	itoa(mqtt_port,buff,10);
	strcat(bsend,buff);
	strcat(bsend,"\" min=\"0\" max=\"65535\" size=\"5\">Port &emsp;<input name=\"smqid\" value=\"");
	if (MQTT_USER[0]) strcat(bsend,MQTT_USER);
	strcat(bsend,"\"size=\"15\">Login &emsp;<input type=\"password\" input name=\"smqpsw\" value=\"");
	if (MQTT_PASSWORD[0]) strcat(bsend,MQTT_PASSWORD);
	strcat(bsend,"\"size=\"19\">Password</br><input type=\"checkbox\" name=\"chk1\" value=\"1\"");
	if (FDHass) strcat(bsend,"checked");
	strcat(bsend,"> Hass Discovery&emsp;<input type=\"checkbox\" name=\"chk2\" value=\"2\"");
	if (ftrufal) strcat(bsend,"checked");
        strcat(bsend,"> \"true/false\" Response(\"ON/OFF\" only if Hass Discovery)</br>");
	strcat(bsend,"<h3>System Setting</h3></br><input name=\"sespnum\" type=\"number\" value=\"");
	itoa(espnrfnum,buff,10);
	strcat(bsend,buff);
	strcat(bsend,"\" min=\"1\" max=\"9\" size=\"2\">espNRF Number &emsp;<input name=\"timzon\" type=\"number\" value=\"");
	uint8_t TmZn = TimeZone;
	if (TmZn >127) {
	TmZn = ~TmZn;
	TmZn++;
	strcat(bsend,"-");
	}
	itoa(TmZn,buff,10);
	strcat(bsend,buff);
	strcat(bsend,"\" min=\"-12\" max=\"12\" size=\"3\">GMT Timezone &emsp;<input name=\"nrfrsp\" type=\"number\" value=\"");
	itoa(NRF_RESET_PIN,buff,10);
	strcat(bsend,buff);
	strcat(bsend,"\" min=\"0\" max=\"16\" size=\"2\">NRF24LE1 reset gpio &emsp;<select name=\"wmode\"><option ");
	if (wmode == 0) strcat(bsend,"selected ");
        strcat(bsend,"value=\"0\">NRF24LE1 only</option><option ");
	if (wmode == 1) strcat(bsend,"selected ");
        strcat(bsend,"value=\"1\">NRF24LE1 + 4 switches</option><option ");
	if (wmode == 2) strcat(bsend,"selected ");
        strcat(bsend,"value=\"2\">NRF24LE1 + Lock</option><option ");
	if (wmode == 3) strcat(bsend,"selected ");
	strcat(bsend,"value=\"3\">NRF24LE1 + Cover</option></select>Working mode</br>");
	strcat(bsend,"<input type=\"checkbox\" name=\"mqtdel\" value=\"1\"");
	strcat(bsend,"> Delete Mqtt topics &emsp;");
	strcat(bsend,"<input type=\"checkbox\" name=\"chk0\" value=\"1\"");
	strcat(bsend,"> Format NVS area</br>");
	if (prwmode == 0) {
/*
	NAMONOF1[0] = 0;
	NAMONOF2[0] = 0;
	NAMONOF3[0] = 0;
	NAMONOF4[0] = 0;
*/

	} else if (prwmode == 1) {
	strcat(bsend,"<h3>Switches Setting</h3></br><input name=\"ssnf1id\" value=\"");
	if (NAMONOF1[0]) strcat(bsend,NAMONOF1);
        strcat(bsend,"\" size=\"32\">Relay1 Name &emsp;<input name=\"ssnf1rp\" type=\"number\" value=\"");
	itoa(RELAY1,buff,10);
	strcat(bsend,buff);
	strcat(bsend,"\" min=\"0\" max=\"16\" size=\"2\">Relay1 gpio &emsp;<input name=\"ssnf1bp\" value=\"");
	itoa(BUTTON1,buff,10);
	strcat(bsend,buff);
	strcat(bsend,"\" min=\"0\" max=\"16\" size=\"2\">Button1 gpio</br><input name=\"ssnf2id\" value=\"");
	if (NAMONOF2[0]) strcat(bsend,NAMONOF2);
        strcat(bsend,"\" size=\"32\">Relay2 Name &emsp;<input name=\"ssnf2rp\" type=\"number\" value=\"");
	itoa(RELAY2,buff,10);
	strcat(bsend,buff);
	strcat(bsend,"\" min=\"0\" max=\"16\" size=\"2\">Relay2 gpio &emsp;<input name=\"ssnf2bp\" value=\"");
	itoa(BUTTON2,buff,10);
	strcat(bsend,buff);
	strcat(bsend,"\" min=\"0\" max=\"16\" size=\"2\">Button2 gpio</br><input name=\"ssnf3id\" value=\"");
	if (NAMONOF3[0]) strcat(bsend,NAMONOF3);
        strcat(bsend,"\" size=\"32\">Relay3 Name &emsp;<input name=\"ssnf3rp\" type=\"number\" value=\"");
	itoa(RELAY3,buff,10);
	strcat(bsend,buff);
	strcat(bsend,"\" min=\"0\" max=\"16\" size=\"2\">Relay3 gpio &emsp;<input name=\"ssnf3bp\" value=\"");
	itoa(BUTTON3,buff,10);
	strcat(bsend,buff);
	strcat(bsend,"\" min=\"0\" max=\"16\" size=\"2\">Button3 gpio</br><input name=\"ssnf4id\" value=\"");
	if (NAMONOF4[0]) strcat(bsend,NAMONOF4);
        strcat(bsend,"\" size=\"32\">Relay4 Name &emsp;<input name=\"ssnf4rp\" type=\"number\" value=\"");
	itoa(RELAY4,buff,10);
	strcat(bsend,buff);
	strcat(bsend,"\" min=\"0\" max=\"16\" size=\"2\">Relay4 gpio &emsp;<input name=\"ssnf4bp\" value=\"");
	itoa(BUTTON4,buff,10);
	strcat(bsend,buff);
	strcat(bsend,"\" min=\"0\" max=\"16\" size=\"2\">Button4 gpio</br></br>");
	strcat(bsend,"<select name=\"lvout\"><option ");
	if (lvout == 0) strcat(bsend,"selected ");
        strcat(bsend,"value=\"0\">Active_High</option><option ");
	if (lvout == 1) strcat(bsend,"selected ");
        strcat(bsend,"value=\"1\">Active_Low</option></select>Outputs</br>");

	} else if (prwmode == 2) {	
	strcat(bsend,"<h3>Lock Setting</h3></br><input name=\"ssnf1id\" value=\"");
	if (NAMONOF1[0]) strcat(bsend,NAMONOF1);
        strcat(bsend,"\" size=\"32\">Lock Name &emsp;<input name=\"ssnf1rp\" type=\"number\" value=\"");
	itoa(RELAY1,buff,10);
	strcat(bsend,buff);
	strcat(bsend,"\" min=\"0\" max=\"16\" size=\"2\">Lock Relay gpio &emsp;<input name=\"ssnf1bp\" value=\"");
	itoa(BUTTON1,buff,10);
	strcat(bsend,buff);
	strcat(bsend,"\" min=\"0\" max=\"16\" size=\"2\">Lock Sensor gpio</br>");
	strcat(bsend,"<select name=\"lvout\"><option ");
	if (lvout == 0) strcat(bsend,"selected ");
        strcat(bsend,"value=\"0\">Active_High</option><option ");
	if (lvout == 1) strcat(bsend,"selected ");
        strcat(bsend,"value=\"1\">Active_Low</option></select>Output & Sensor</br>");

	} else if (prwmode == 3) {	
	strcat(bsend,"<h3>Cover Setting</h3></br><input name=\"ssnf1id\" value=\"");
	if (NAMONOF1[0]) strcat(bsend,NAMONOF1);
        strcat(bsend,"\" size=\"32\">Cover Name &emsp;<input name=\"ssnf1rp\" type=\"number\" value=\"");
	itoa(RELAY1,buff,10);
	strcat(bsend,buff);
	strcat(bsend,"\" min=\"0\" max=\"16\" size=\"2\">Open Relay gpio &emsp;<input name=\"ssnf2rp\" value=\"");
	itoa(RELAY2,buff,10);
	strcat(bsend,buff);
	strcat(bsend,"\" min=\"0\" max=\"16\" size=\"2\">Close Relay gpio &emsp;<input name=\"ssnfdly\" value=\"");
	itoa(CDELAY,buff,10);
	strcat(bsend,buff);
	strcat(bsend,"\" min=\"0\" max=\"255\" size=\"3\">Delay</br><input name=\"ssnf1bp\" value=\"");
	itoa(BUTTON1,buff,10);
	strcat(bsend,buff);
	strcat(bsend,"\" min=\"0\" max=\"16\" size=\"2\">Open Button gpio &emsp;<input name=\"ssnf2bp\" value=\"");
	itoa(BUTTON2,buff,10);
	strcat(bsend,buff);
	strcat(bsend,"\" min=\"0\" max=\"16\" size=\"2\">Close Button gpio</br>");
/*
	strcat(bsend,"\" min=\"0\" max=\"16\" size=\"2\">Close Button gpio &emsp;<input name=\"ssnf3bp\" value=\"");
	itoa(BUTTON3,buff,10);
	strcat(bsend,buff);
	strcat(bsend,"\" min=\"0\" max=\"16\" size=\"2\">Open Sensor gpio &emsp;<input name=\"ssnf4bp\" value=\"");
	itoa(BUTTON4,buff,10);
	strcat(bsend,buff);
	strcat(bsend,"\" min=\"0\" max=\"16\" size=\"2\">Close Sensor gpio</br>");
*/
	strcat(bsend,"<select name=\"lvout\"><option ");
	if (lvout == 0) strcat(bsend,"selected ");
        strcat(bsend,"value=\"0\">Active_High</option><option ");
	if (lvout == 1) strcat(bsend,"selected ");
        strcat(bsend,"value=\"1\">Active_Low</option></select>Outputs</br>");
	}
	strcat(bsend,"<h3>Store setting then press SAVE. espNRF Gate will restart</h3><br/>");
	strcat(bsend,"<input type=SUBMIT value=\"Save settings\"></form><form method=\"POST\" action=\"/setignore\">");
	strcat(bsend,"<input type=SUBMIT value=\"Cancel\"></form></body></html>");

//itoa(strlen(bsend),buff,10);
//strcat(bsend,buff);

	httpd_resp_send(req, bsend, strlen(bsend));
	free(bsend);
	}
    return ESP_OK;
}

static const httpd_uri_t psetting = {
	.uri       = "/setting",
	.method    = HTTP_GET,
	.handler   = psetting_get_handler,
	.user_ctx  = NULL
};

bool pinvalid(uint8_t pin) {
	bool retc = true;
	if (pin > 16) retc = false;
	if ((pin > 5) && (pin < 12)) retc = false;
	return retc;
}

static esp_err_t psetsave_get_handler(httpd_req_t *req)
{
	char buf1[1024] = {0};
	char buf2[16] = {0};
	char buf3[16] = {0};
	uint8_t pintemp;
//save ip from header
	char bufip[32] = {0};
	int buf_len;
	buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
	if (buf_len > 1) {
	if (buf_len >31) buf_len = 31;
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", bufip, buf_len) == ESP_OK) {
        }
	}
	int  ret;
	ret = httpd_req_recv(req,buf1,1024);
	if ( ret > 0 ) {
//ESP_LOGI(TAG, "Buf: '%s'", buf1);
/*

in buf1 after httpd_req_recv string like below
swfid=wifiname&swfpsw=wifipassword&smqsrv=192.168.1.10&smqid=esp&
smqpsw=esp&devnam=&rlight=255&glight=255&blight=255&chk2=2

*/
	strcpy(buf2,"swfid");
	parsuri(buf1,WIFI_SSID,buf2,1024,33);
	strcpy(buf2,"ssnf1id");
	parsuri(buf1,NAMONOF1,buf2,1024,33);
	strcpy(buf2,"ssnf2id");
	parsuri(buf1,NAMONOF2,buf2,1024,33);
	strcpy(buf2,"ssnf3id");
	parsuri(buf1,NAMONOF3,buf2,1024,33);
	strcpy(buf2,"ssnf4id");
	parsuri(buf1,NAMONOF4,buf2,1024,33);
	strcpy(buf2,"swfpsw");
	parsuri(buf1,WIFI_PASSWORD,buf2,1024,65);
	strcpy(buf2,"smqsrv");
	parsuri(buf1,MQTT_SERVER,buf2,1024,16);
	strcpy(buf2,"smqid");
	parsuri(buf1,MQTT_USER,buf2,1024,16);
	strcpy(buf2,"smqpsw");
	parsuri(buf1,MQTT_PASSWORD,buf2,1024,20);
	strcpy(buf2,"smqprt");
	parsuri(buf1,buf3,buf2,1024,8);
	mqtt_port = atoi(buf3);
	strcpy(buf2,"sespnum");
	parsuri(buf1,buf3,buf2,1024,4);
	espnrfnum = atoi(buf3);
	strcpy(buf2,"timzon");
	parsuri(buf1,buf3,buf2,1024,4);
	TimeZone = atoi(buf3);
	strcpy(buf2,"lvout");
	parsuri(buf1,buf3,buf2,1024,4);
	lvout = atoi(buf3);
	buf3[0] = 0;
	strcpy(buf2,"chk1");
	parsuri(buf1,buf3,buf2,1024,2);
	FDHass = 0;
	if (buf3[0] == 0x31) FDHass = 1;
	buf3[0] = 0;
	strcpy(buf2,"chk2");
	parsuri(buf1,buf3,buf2,1024,2);
	ftrufal = 0;
	if (buf3[0] == 0x32) ftrufal = 1;
	if (FDHass) ftrufal = 0;
	buf3[0] = 0;
	strcpy(buf2,"wmode");
	parsuri(buf1,buf3,buf2,1024,4);
	wmode = atoi(buf3);
	buf3[0] = 0;
	strcpy(buf2,"nrfrsp");
	parsuri(buf1,buf3,buf2,1024,4);
        pintemp = atoi(buf3);
	if (pinvalid(pintemp)) NRF_RESET_PIN = pintemp;

	if (wmode) {
	buf3[0] = 0;
	strcpy(buf2,"ssnf1rp");
	parsuri(buf1,buf3,buf2,1024,4);
        pintemp = atoi(buf3);
	if (buf3[0] && pinvalid(pintemp)) RELAY1 = pintemp;
	buf3[0] = 0;
	strcpy(buf2,"ssnf2rp");
	parsuri(buf1,buf3,buf2,1024,4);
        pintemp = atoi(buf3);
	if (buf3[0] && pinvalid(pintemp)) RELAY2 = pintemp;
	buf3[0] = 0;
	strcpy(buf2,"ssnf3rp");
	parsuri(buf1,buf3,buf2,1024,4);
        pintemp = atoi(buf3);
	if (buf3[0] && pinvalid(pintemp)) RELAY3 = pintemp;
	buf3[0] = 0;
	strcpy(buf2,"ssnf4rp");
	parsuri(buf1,buf3,buf2,1024,4);
        pintemp = atoi(buf3);
	if (buf3[0] && pinvalid(pintemp)) RELAY4 = pintemp;
	buf3[0] = 0;
	strcpy(buf2,"ssnf1bp");
	parsuri(buf1,buf3,buf2,1024,4);
        pintemp = atoi(buf3);
	if (buf3[0] && pinvalid(pintemp)) BUTTON1 = pintemp;
	buf3[0] = 0;
	strcpy(buf2,"ssnf2bp");
	parsuri(buf1,buf3,buf2,1024,4);
        pintemp = atoi(buf3);
	if (buf3[0] && pinvalid(pintemp)) BUTTON2 = pintemp;
	buf3[0] = 0;
	strcpy(buf2,"ssnf3bp");
	parsuri(buf1,buf3,buf2,1024,4);
        pintemp = atoi(buf3);
	if (buf3[0] && pinvalid(pintemp)) BUTTON3 = pintemp;
	buf3[0] = 0;
	strcpy(buf2,"ssnf4bp");
	parsuri(buf1,buf3,buf2,1024,4);
        pintemp = atoi(buf3);
	if (buf3[0] && pinvalid(pintemp)) BUTTON4 = pintemp;
	buf3[0] = 0;
	strcpy(buf2,"ssnfdly");
	parsuri(buf1,buf3,buf2,1024,4);
        CDELAY = atoi(buf3);
	if (!RELAY1) NAMONOF1[0] = 0;
	if (!RELAY2) NAMONOF2[0] = 0;
	if (!RELAY3) NAMONOF3[0] = 0;
	if (!RELAY4) NAMONOF4[0] = 0;
	if (!CDELAY) CDELAY = 180;

	}

	buf3[0] = 0;
	strcpy(buf2,"chk0");
	parsuri(buf1,buf3,buf2,1024,2);
	if (buf3[0] == 0x31) {
//	nvs_flash_erase();
//	nvs_flash_init();
	}

	buf3[0] = 0;
	strcpy(buf2,"mqtdel");
	parsuri(buf1,buf3,buf2,1024,2);
	if ((buf3[0] == 0x31) && (wmode == prwmode)) {
	setsmqtt(0);
	strcpy(buf2, "nrf");
	if (espnrfnum)  {
	itoa(espnrfnum,buf3,10);
	strcat(buf2, buf3);
	}
	if (FDHass) {
	strcpy(buf1,"homeassistant/sensor/");
	strcat(buf1,buf2);
	strcat(buf1,"/#");
	mqtdel = 20;
	esp_mqtt_client_subscribe(mqttclient, buf1, 0);
	while (--mqtdel > 1) vTaskDelay(100 / portTICK_PERIOD_MS);
	strcpy(buf1,"homeassistant/switch/");
	strcat(buf1,buf2);
	strcat(buf1,"/#");
	mqtdel = 20;
	esp_mqtt_client_subscribe(mqttclient, buf1, 0);
	while (--mqtdel > 1) vTaskDelay(100 / portTICK_PERIOD_MS);
	strcpy(buf1,"homeassistant/lock/");
	strcat(buf1,buf2);
	strcat(buf1,"/#");
	mqtdel = 20;
	esp_mqtt_client_subscribe(mqttclient, buf1, 0);
	while (--mqtdel > 1) vTaskDelay(100 / portTICK_PERIOD_MS);
	strcpy(buf1,"homeassistant/cover/");
	strcat(buf1,buf2);
	strcat(buf1,"/#");
	mqtdel = 20;
	esp_mqtt_client_subscribe(mqttclient, buf1, 0);
	while (--mqtdel > 1) vTaskDelay(100 / portTICK_PERIOD_MS);
	}
	strcpy(buf1,buf2);
	strcat(buf1,"/#");
	mqtdel = 20;
	esp_mqtt_client_subscribe(mqttclient, buf1, 0);
	while (--mqtdel > 1) vTaskDelay(100 / portTICK_PERIOD_MS);

	if (espnrfnum != espnrfnumo)  {
	strcpy(buf2, "nrf");
	if (espnrfnumo)  {
	itoa(espnrfnumo,buf3,10);
	strcat(buf2, buf3);
	}
	if (FDHass) {
	strcpy(buf1,"homeassistant/sensor/");
	strcat(buf1,buf2);
	strcat(buf1,"/#");
	mqtdel = 20;
	esp_mqtt_client_subscribe(mqttclient, buf1, 0);
	while (--mqtdel > 1) vTaskDelay(100 / portTICK_PERIOD_MS);
	strcpy(buf1,"homeassistant/switch/");
	strcat(buf1,buf2);
	strcat(buf1,"/#");
	mqtdel = 20;
	esp_mqtt_client_subscribe(mqttclient, buf1, 0);
	while (--mqtdel > 1) vTaskDelay(100 / portTICK_PERIOD_MS);
	strcpy(buf1,"homeassistant/lock/");
	strcat(buf1,buf2);
	strcat(buf1,"/#");
	mqtdel = 20;
	esp_mqtt_client_subscribe(mqttclient, buf1, 0);
	while (--mqtdel > 1) vTaskDelay(100 / portTICK_PERIOD_MS);
	strcpy(buf1,"homeassistant/cover/");
	strcat(buf1,buf2);
	strcat(buf1,"/#");
	mqtdel = 20;
	esp_mqtt_client_subscribe(mqttclient, buf1, 0);
	while (--mqtdel > 1) vTaskDelay(100 / portTICK_PERIOD_MS);
	}
	strcpy(buf1,buf2);
	strcat(buf1,"/#");
	mqtdel = 20;
	esp_mqtt_client_subscribe(mqttclient, buf1, 0);
	while (--mqtdel > 1) vTaskDelay(100 / portTICK_PERIOD_MS);


	}
	}




// write nvs
	nvs_handle my_handle;
	ret = nvs_open("storage", NVS_READWRITE, &my_handle);
	if (ret == ESP_OK) {

	nvs_set_u16(my_handle, "smqprt", mqtt_port);
	nvs_set_u8(my_handle,  "sespnum", espnrfnum);
	nvs_set_u8(my_handle,  "timzon", TimeZone);
	nvs_set_u8(my_handle,  "lvout", lvout);
	nvs_set_u8(my_handle,  "chk1",  FDHass);
	nvs_set_u8(my_handle,  "chk2",  ftrufal);
	nvs_set_u8(my_handle,  "wmode",  wmode);
	nvs_set_u8(my_handle, "nrfrsp", NRF_RESET_PIN);

	nvs_set_u8(my_handle,  "ssnf1rp", RELAY1);
	nvs_set_u8(my_handle,  "ssnf2rp", RELAY2);
	nvs_set_u8(my_handle,  "ssnf3rp", RELAY3);
	nvs_set_u8(my_handle,  "ssnf4rp", RELAY4);
	nvs_set_u8(my_handle,  "ssnf1bp", BUTTON1);
	nvs_set_u8(my_handle,  "ssnf2bp", BUTTON2);
	nvs_set_u8(my_handle,  "ssnf3bp", BUTTON3);
	nvs_set_u8(my_handle,  "ssnf4bp", BUTTON4);
	nvs_set_u8(my_handle,  "ssnfdly", CDELAY);

	nvs_set_str(my_handle, "swfid", WIFI_SSID);
	nvs_set_str(my_handle, "swfpsw", WIFI_PASSWORD);
	nvs_set_str(my_handle, "smqsrv", MQTT_SERVER);
	nvs_set_str(my_handle, "smqid", MQTT_USER);
	nvs_set_str(my_handle, "smqpsw", MQTT_PASSWORD);
	nvs_set_str(my_handle, "ssnf1id", NAMONOF1);
	nvs_set_str(my_handle, "ssnf2id", NAMONOF2);
	nvs_set_str(my_handle, "ssnf3id", NAMONOF3);
	nvs_set_str(my_handle, "ssnf4id", NAMONOF4);

        ret = nvs_commit(my_handle);
	if (ret != ESP_OK) {
//	ESP_LOGE(TAG, "NVS write error");
}
// Close nvs
	nvs_close(my_handle);
	}
	}
	if (wmode == prwmode) {
	strcpy (buf1,"<!DOCTYPE html><html><head><title>espnrf</title><meta name='viewport' content='width=device-width, initial-scale=1.0'><meta http-equiv=\"refresh\" content=\"3;URL=http://");
	strcat(buf1, bufip);
	strcat(buf1,"\">");
	strcat(buf1,"</head><body>Setting saved. Rebooting...</body></html>");
	httpd_resp_send(req, buf1, strlen(buf1));

//	ESP_LOGI(TAG, "Prepare to restart system!");
	vTaskDelay(500 / portTICK_PERIOD_MS);
	esp_restart();
	} else {
	prwmode = wmode;
	httpd_resp_set_status(req, "303 See Other");
	httpd_resp_set_hdr(req, "Location", "/setting");
	httpd_resp_send(req, NULL, 0);  // Response body can be empty
	}
    return ESP_OK;
}
static const httpd_uri_t psetsave = {
	.uri       = "/setsave",
	.method    = HTTP_POST,
	.handler   = psetsave_get_handler,
	.user_ctx  = NULL
};

static esp_err_t psetignore_get_handler(httpd_req_t *req)
{
	httpd_resp_set_status(req, "303 See Other");
	httpd_resp_set_hdr(req, "Location", "/");
	httpd_resp_send(req, NULL, 0);  // Response body can be empty
    return ESP_OK;
}


static const httpd_uri_t psetignore = {
	.uri       = "/setignore",
	.method    = HTTP_POST,
	.handler   = psetignore_get_handler,
	.user_ctx  = NULL
};

/* HTTP GET update nrf handler */
static esp_err_t ploadnrf_get_handler(httpd_req_t *req)
{
	httpd_resp_send(req, nrfserverIndex, strlen(nrfserverIndex));
    return ESP_OK;
}
static const httpd_uri_t ploadnrf = {
	.uri       = "/loadnrf",
	.method    = HTTP_GET,
	.handler   = ploadnrf_get_handler,
	.user_ctx  = NULL
};

/* HTTP GET updating nrf handler */
static esp_err_t ploadingnrf_get_handler(httpd_req_t *req)
{
	char otabuf[otabufsize] ={0};
	char filnam[128] ={0};
        int  data_read = 0;
	int  otabufoffs = 0;
	bool image_header_was_checked = false;
	bool ota_running = true;
//save ip from header
	char bufip[32] = {0};
	int buf_len;
	buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
	if (buf_len > 1) {
	if (buf_len >31) buf_len = 31;
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", bufip, buf_len) == ESP_OK) {
        }
	}
	/*deal with all receive packet*/
// read loop
	while (ota_running) {
	otabufoffs = 0;
//read max 2048 bytes
        data_read = httpd_req_recv(req, otabuf, otabufsize);
        if (data_read < 0) {
//	ESP_LOGE(TAG, "Error: data read error");
	ota_running = false;
	otabufoffs = 0;
        } else if (data_read > 0) {
	if (image_header_was_checked == false) {
//uart_write_bytes(UART_NUM_0, otabuf, otabufsize);
// if first read check and remove POST header
/*
in otabuf after httpd_req_recv string like below

------WebKitFormBoundary2ZdUwM7CAu6TtDPq
Content-Disposition: form-data; name="update"; filename="espnrf.bin"
Content-Type: application/octet-stream\r\n\r\n
*/

//check "Content-Disposition" string in received data
	otabufoffs = parsoff(otabuf,"Content-Disposition", otabufsize);
	if (!otabufoffs) {
//	ESP_LOGE(TAG, "Content-Disposition not found");
	ota_running = false;
	otabufoffs = 0;
	data_read = 0;
	}

// save filename
	otabufoffs = parsoff(otabuf,"filename=", otabufsize);
	mystrcpy(filnam, otabuf+otabufoffs, 127);
// search for data begin
	otabufoffs = parsoff(otabuf,"application/octet-stream\r\n\r", otabufsize);
	if (!otabufoffs) {
//	ESP_LOGE(TAG, "application/octet-stream not found");
	ota_running = false;
	otabufoffs = 0;
	data_read = 0;
	} else otabufoffs++;
//ESP_LOGI(TAG, "Loading filename: %s",filnam);
	image_header_was_checked = true;
	UPLIDX = 0;
	} 
	if (data_read > 0)  {
	if ((UPLIDX + data_read - otabufoffs) <= UPLBUF_SIZE) {
	memcpy(UPLBUF+UPLIDX, otabuf+otabufoffs, data_read-otabufoffs);
	UPLIDX = UPLIDX + data_read - otabufoffs;
	if (UPLIDX == UPLBUF_SIZE){
	ota_running = false;
	otabufoffs = 0;
	data_read = 0;
	UPLIDX = 0;
	f_lfwnrf = true;    // load fw variable
	f_rfwnrf = true;
	f_ffwnrf = false;
	f_sfwnrf = true;
	}
	} else if (UPLIDX < UPLBUF_SIZE) {
	memcpy(UPLBUF+UPLIDX, otabuf+otabufoffs, UPLBUF_SIZE-UPLIDX);
	ota_running = false;
	otabufoffs = 0;
	data_read = 0;
	UPLIDX = 0;
	f_lfwnrf = true;    // load fw variable
	f_rfwnrf = true;
	f_ffwnrf = false;
	f_sfwnrf = true;
	} else {
	ota_running = false;
	otabufoffs = 0;
	data_read = 0;
	UPLIDX = 0;
	}
	}
	} else if (data_read == 0) {
	ota_running = false;
	otabufoffs = 0;
	data_read = 0;
	UPLIDX = 0;
	}
}
//ESP_LOGI(TAG, "Total binary data length: %x", UPLIDX);
	httpd_resp_set_status(req, "303 See Other");
	httpd_resp_set_hdr(req, "Location", "/");
	httpd_resp_send(req, NULL, 0);  // Response body can be empty
    return ESP_OK;
}
static const httpd_uri_t ploadingnrf = {
	.uri       = "/loadingnrf",
	.method    = HTTP_POST,
	.handler   = ploadingnrf_get_handler,
	.user_ctx  = NULL
};




/* HTTP GET update esp handler */
static esp_err_t ploadesp_get_handler(httpd_req_t *req)
{
	httpd_resp_send(req, espserverIndex, strlen(espserverIndex));
    return ESP_OK;
}
static const httpd_uri_t ploadesp = {
	.uri       = "/loadesp",
	.method    = HTTP_GET,
	.handler   = ploadesp_get_handler,
	.user_ctx  = NULL
};


/* HTTP GET updating esp handler */
static esp_err_t ploadingesp_get_handler(httpd_req_t *req)
{
//	f_update = true;
	char otabuf[otabufsize] ={0};
	char filnam[128] ={0};
        int data_read = 0;
	int  otabufoffs = 0;
	esp_err_t err = 0;
	int binary_file_length = 0;
	bool image_header_was_checked = false;
	bool ota_running = true;
	esp_ota_handle_t update_handle = 0 ;
//save ip from header
	char bufip[32] = {0};
	int buf_len;
	buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
	if (buf_len > 1) {
	if (buf_len >31) buf_len = 31;
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", bufip, buf_len) == ESP_OK) {
        }
	}
//	ESP_LOGI(TAG, "Starting OTA");
	const esp_partition_t *pupdate = esp_ota_get_next_update_partition(NULL);
	/*deal with all receive packet*/
// read loop
	while (ota_running) {
	otabufoffs = 0;
//read max 2048 bytes
        data_read = httpd_req_recv(req, otabuf, otabufsize);
        if (data_read < 0) {
//	ESP_LOGE(TAG, "Error: data read error");
	ota_running = false;
	otabufoffs = 0;
        } else if (data_read > 0) {
	if (image_header_was_checked == false) {
// if first read check and remove POST header
/*
in otabuf after httpd_req_recv string like below

------WebKitFormBoundary2ZdUwM7CAu6TtDPq
Content-Disposition: form-data; name="update"; filename="espnrf.bin"
Content-Type: application/octet-stream\r\n\r\n
*/

//check "Content-Disposition" string in received data
	otabufoffs = parsoff(otabuf,"Content-Disposition", otabufsize);
	if (!otabufoffs) {
//	ESP_LOGE(TAG, "Content-Disposition not found");
	ota_running = false;
	otabufoffs = 0;
	data_read = 0;
	}

// save filename
	otabufoffs = parsoff(otabuf,"filename=", otabufsize);
	mystrcpy(filnam, otabuf+otabufoffs, 127);
// search for data begin
	otabufoffs = parsoff(otabuf,"application/octet-stream\r\n\r\n", otabufsize);
	if (!otabufoffs) otabufoffs = parsoff(otabuf,"application/macbinary\r\n\r\n", otabufsize);
	if (!otabufoffs) {
//	ESP_LOGE(TAG, "application/octet-stream not found");
	ota_running = false;
	otabufoffs = 0;
	data_read = 0;
	}

//esp_log_buffer_hex(TAG, otabuf, 128);
//	ESP_LOGI(TAG, "Loading filename: %s",filnam);

	image_header_was_checked = true;
	err = esp_ota_begin(pupdate, OTA_SIZE_UNKNOWN, &update_handle);
	if (err != ESP_OK) {
//	ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
	ota_running = false;
	otabufoffs = 0;
	data_read = 0;
	}
//	ESP_LOGI(TAG, "esp_ota_begin succeeded");
	} 
	if (data_read > 0)  {
	err = esp_ota_write(update_handle, (const void *)otabuf+otabufoffs, data_read-otabufoffs);
	if (err != ESP_OK) {
	ota_running = false;
	otabufoffs = 0;
	data_read = 0;
	}

	}
	binary_file_length = binary_file_length  + data_read - otabufoffs; 
//ESP_LOGI(TAG, "Written image length %x, offset %x", binary_file_length, otabufoffs);
	} else if (data_read == 0) {
	ota_running = false;
	otabufoffs = 0;
	data_read = 0;
	}
}
//
//	ESP_LOGI(TAG, "Total Write binary data length: %x", binary_file_length);
	
	err = esp_ota_end(update_handle);
	strcpy (otabuf,"<!DOCTYPE html><html><head><title>espnrf</title><meta name='viewport' content='width=device-width, initial-scale=1.0'><meta http-equiv=\"refresh\" content=\"3;URL=http://");
	strcat(otabuf, bufip);
	strcat(otabuf,"\">");
	strcat(otabuf,"</head><body>Update ");
	if (err != ESP_OK) {
	if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
//	ESP_LOGE(TAG, "Image validation failed, image is corrupted");
	}
//	ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
	strcat(otabuf,"failed");
	} else {
//	ESP_LOGI(TAG, "esp_ota_end ok!");
	strcat(otabuf,"ok");
	}
	strcat(otabuf,". Rebooting...</body></html>");
	httpd_resp_send(req, otabuf, strlen(otabuf));

	err = esp_ota_set_boot_partition(pupdate);
	if (err != ESP_OK) {
//	ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
	ota_running = false;
	}
//	ESP_LOGI(TAG, "Prepare to restart system!");
	vTaskDelay(500 / portTICK_PERIOD_MS);
	esp_restart();
    return ESP_OK;
}
static const httpd_uri_t ploadingesp = {
	.uri       = "/loadingesp",
	.method    = HTTP_POST,
	.handler   = ploadingesp_get_handler,
	.user_ctx  = NULL
};





//*************************************************
httpd_handle_t start_webserver(void)
{
	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.max_uri_handlers = 16;
//	config.max_resp_headers =16;
	config.stack_size = 10240;

    // Start the httpd server
/*
	ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
	ESP_LOGI(TAG, "Max URI handlers: '%d'", config.max_uri_handlers);
	ESP_LOGI(TAG, "Max Open Sessions: '%d'", config.max_open_sockets);
	ESP_LOGI(TAG, "Max Header Length: '%d'", HTTPD_MAX_REQ_HDR_LEN);
	ESP_LOGI(TAG, "Max URI Length: '%d'", HTTPD_MAX_URI_LEN);
	ESP_LOGI(TAG, "Max Stack Size: '%d'", config.stack_size);
*/
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
//ESP_LOGI(TAG, "Registering URI handlers");
	httpd_register_uri_handler(server, &pmain);
//	httpd_register_uri_handler(server, &plive);
	httpd_register_uri_handler(server, &pwebcmd);
	httpd_register_uri_handler(server, &pdatatxt);
	httpd_register_uri_handler(server, &presetnrf);
	httpd_register_uri_handler(server, &presetesp);
	httpd_register_uri_handler(server, &psetting);
	httpd_register_uri_handler(server, &psetsave);
	httpd_register_uri_handler(server, &psetignore);
	httpd_register_uri_handler(server, &ploadnrf);
	httpd_register_uri_handler(server, &ploadesp);
	httpd_register_uri_handler(server, &ploadingesp);
	httpd_register_uri_handler(server, &ploadingnrf);

        return server;
    }

//    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}
static httpd_handle_t server = NULL;



//******************* Main **********************
void app_main()
{
	esp_err_t ret;
	wifi_ap_record_t wifidata;
	bool retres;
	uint32_t pinbitmask;
//empty
//	f_update = false;
	s_retry_num = 0;
	floop = 0;
	NumWfConn = 0;
	mqttConnected = false;
	MQTT_USER[0] = 0;
	MQTT_PASSWORD[0] = 0;
	MQTT_SERVER[0] = 0;
	WIFI_SSID[0] = 0;
	WIFI_PASSWORD[0] = 0;
	bufferln[0] = 0;
	bufferln1[0] = 0;
	cptbufln = 0;
	cptbufrx = 0;
	lenbufrx = 0;
	tuptime = 0;
	lockcount = 0;
	UPLIDX = 0;
	MQTT_DATA[0] = 0;
	MQTT_TOPIC[0] = 0;
	TimeZone = 0;
	FDHass = 0;
	RELAY1 = 12;
	RELAY2 = 14;
	RELAY3 = 13;
	RELAY4 = 15;
	BUTTON1 = 4;
	BUTTON2 = 5;
	BUTTON3 = 16;
	BUTTON4 = 0;
	CDELAY = 60;
	NAMONOF1[0] = 0;
	NAMONOF2[0] = 0;
	NAMONOF3[0] = 0;
	NAMONOF4[0] = 0;
	wmode = 0;
	lvout = 0;
	NRF_RESET_PIN = 2;  // ESP pin connected to NRF24 reset pin - GPIO2 = D4
//
	f_lfwnrf = false;   // load fw send disabled
	f_rfwnrf = true;    // no waiting for response
	f_ffwnrf = false;   // load fw error
	f_sfwnrf = false;
	f_tfwnrf = 0;
//
	strcpy(STRON,"ON");
	strcpy(STROF,"OFF");
	strcpy(WIFI_SSID, "espnrf");
	strcpy(WIFI_PASSWORD, "12345678");

//Initialize NVS
	ret = nvs_flash_init();
// read nvs
	nvs_handle my_handle;
	ret = nvs_open("storage", NVS_READWRITE, &my_handle);
	if (ret == ESP_OK) {
	nvs_get_u16(my_handle, "smqprt", &mqtt_port);
	nvs_get_u8(my_handle, "sespnum", &espnrfnum);
	nvs_get_u8(my_handle, "timzon", &TimeZone);
	nvs_get_u8(my_handle, "chk1",   &FDHass);
	nvs_get_u8(my_handle, "chk2",   &ftrufal);
	nvs_get_u8(my_handle, "wmode",  &wmode);
	nvs_get_u8(my_handle, "lvout",  &lvout);
	nvs_get_u8(my_handle, "nrfrsp", &NRF_RESET_PIN);
	nvs_get_u8(my_handle,  "ssnf1rp", &RELAY1);
	nvs_get_u8(my_handle,  "ssnf2rp", &RELAY2);
	nvs_get_u8(my_handle,  "ssnf3rp", &RELAY3);
	nvs_get_u8(my_handle,  "ssnf4rp", &RELAY4);
	nvs_get_u8(my_handle,  "ssnf1bp", &BUTTON1);
	nvs_get_u8(my_handle,  "ssnf2bp", &BUTTON2);
	nvs_get_u8(my_handle,  "ssnf3bp", &BUTTON3);
	nvs_get_u8(my_handle,  "ssnf4bp", &BUTTON4);
	nvs_get_u8(my_handle,  "ssnfdly", &CDELAY);
	size_t nvsize = 32;
	nvs_get_str(my_handle,"swfid", WIFI_SSID,&nvsize);
	nvsize = 32;
	nvs_get_str(my_handle, "ssnf1id", NAMONOF1,&nvsize);
	nvsize = 32;
	nvs_get_str(my_handle, "ssnf2id", NAMONOF2,&nvsize);
	nvsize = 32;
	nvs_get_str(my_handle, "ssnf3id", NAMONOF3,&nvsize);
	nvsize = 32;
	nvs_get_str(my_handle, "ssnf4id", NAMONOF4,&nvsize);
	nvsize = 64;
	nvs_get_str(my_handle,"swfpsw", WIFI_PASSWORD,&nvsize);
	nvsize = 16;
	nvs_get_str(my_handle,"smqsrv", MQTT_SERVER,&nvsize);
	nvsize = 16;
	nvs_get_str(my_handle,"smqid", MQTT_USER,&nvsize);
	nvsize = 20;
	nvs_get_str(my_handle,"smqpsw", MQTT_PASSWORD,&nvsize);
// Close nvs
	nvs_close(my_handle);
	}
	prwmode = wmode;
	espnrfnumo = espnrfnum;
	if (!WIFI_SSID[0]) {
	strcpy(WIFI_SSID, "espnrf");
	strcpy(WIFI_PASSWORD, "12345678");
	}
	if (FDHass) ftrufal = 0;
	if (ftrufal) {
	strcpy(STRON,"true");
	strcpy(STROF,"false");
	}
	pinbitmask = 0;
	pinbitmask = (pinbitmask | (1ULL<<NRF_RESET_PIN));

	if (wmode == 1) {
	if (NAMONOF1[0]) pinbitmask = (pinbitmask | (1ULL<<RELAY1));
	if (NAMONOF2[0]) pinbitmask = (pinbitmask | (1ULL<<RELAY2));
	if (NAMONOF3[0]) pinbitmask = (pinbitmask | (1ULL<<RELAY3));
	if (NAMONOF4[0]) pinbitmask = (pinbitmask | (1ULL<<RELAY4));
	} else if (wmode == 2) {
	if (NAMONOF1[0]) pinbitmask = (pinbitmask | (1ULL<<RELAY1));
	} else if (wmode == 3) {
	if (NAMONOF1[0]) pinbitmask = (pinbitmask | (1ULL<<RELAY1) | (1ULL<<RELAY2));
	}
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
//	io_conf.pin_bit_mask = ((1ULL<<RELAY1) | (1ULL<<RELAY2) | (1ULL<<NRF_RESET_PIN));
	io_conf.pin_bit_mask = pinbitmask;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);

	pinbitmask = 0;
	if (wmode == 1) {
	if (NAMONOF1[0]) pinbitmask = (pinbitmask | (1ULL<<BUTTON1));
	if (NAMONOF2[0]) pinbitmask = (pinbitmask | (1ULL<<BUTTON2));
	if (NAMONOF3[0]) pinbitmask = (pinbitmask | (1ULL<<BUTTON3));
	if (NAMONOF4[0]) pinbitmask = (pinbitmask | (1ULL<<BUTTON4));
	} else if (wmode == 2) {
	if (NAMONOF1[0]) pinbitmask = (pinbitmask | (1ULL<<BUTTON1));
	} else if (wmode == 3) {
	if (NAMONOF1[0]) {
	pinbitmask = (pinbitmask | (1ULL<<BUTTON1) | (1ULL<<BUTTON2));
	if (!CDELAY) pinbitmask = (pinbitmask | (1ULL<<BUTTON3) | (1ULL<<BUTTON4));
	}
	}
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_INPUT;
//	io_conf.pin_bit_mask = ((1ULL<<BUTTON1) | (1ULL<<BUTTON2));
	io_conf.pin_bit_mask = pinbitmask;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 1;
	if (pinbitmask) gpio_config(&io_conf);

	if (wmode == 1) {
	if (NAMONOF1[0]) gpio_set_level(RELAY1, lvout);
	if (NAMONOF2[0]) gpio_set_level(RELAY2, lvout);
	if (NAMONOF3[0]) gpio_set_level(RELAY3, lvout);
	if (NAMONOF4[0]) gpio_set_level(RELAY4, lvout);
	} else if (wmode == 2) {
	if (NAMONOF1[0]) gpio_set_level(RELAY1, lvout);
	strcpy(STRON,"UNLOCKED");
	strcpy(STROF,"LOCKED");

	} else if (wmode == 3) {
	stcvpos = 254;
	crcvpos = 254;
	prcvpos = 255;
	if (NAMONOF1[0]) {
	gpio_set_level(RELAY1, lvout);
	gpio_set_level(RELAY2, lvout);
	}
	}

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	gpio_set_level(NRF_RESET_PIN, 0); //reset nrf24
	vTaskDelay(200 / portTICK_PERIOD_MS);

// fill basic parameters
	char tbuff[64];
	strcpy(MQTT_BASE_TOPIC, "nrf");
	itoa(espnrfnum,tbuff,10);
	strcat(MQTT_BASE_TOPIC, tbuff);
	strcpy(MQTT_CLIENT_NAME, "espNRF");
	itoa(espnrfnum,tbuff,10);
	strcat(MQTT_CLIENT_NAME, tbuff);

//Initialize hw timer
//ESP_LOGI(TAG, "Initialize hw_timer for callback");
	hw_timer_init(hw_timer_callback, NULL);
//ESP_LOGI(TAG, "Set hw_timer timing time 8ms with reload");
	hw_timer_alarm_us(8000, true);
//
	gpio_set_level(NRF_RESET_PIN, 1); //start nrf24
//Initialize Wifi
	wifi_init_sta();
//Initialize Mqtt
	if (MQTT_SERVER[0]) mqtt_app_start();
//timezone
/*
to get MSK (GMT + 3) I need to write GMT-3
*/
	char tzbuff[32];
	uint8_t TimZn = TimeZone;
	strcpy(tbuff,"GMT");
	if (TimZn > 127 ) {
	strcat (tbuff,"+");
	TimZn = ~TimZn;
	TimZn++;
	} else strcat (tbuff,"-");
	itoa(TimZn,tzbuff,10);
	strcat(tbuff,tzbuff);
	setenv("TZ", tbuff, 1);
	tzset();
//sntp
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, "pool.ntp.org");
	sntp_init();
// get esp mac addr
	uint8_t macbuf[8];
        tESP8266Addr[0] = 0;
	esp_read_mac(macbuf,0);
	bin2hex(macbuf, tESP8266Addr,6);
	itoa(espnrfnum,tbuff,10);
	strcat (tESP8266Addr,tbuff);
//Initialize http server
/* Start the server for the first time */
	server = start_webserver();
// Configure parameters of an UART driver, communication pins and install the driver
    uart_config_t uart_config = {
        .baud_rate = 38400,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
//	uart_driver_install(UART_NUM_0, UART_BUF_SIZE * 2, 0, 0, NULL);
	uart_driver_install(UART_NUM_0, UART_BUF_SIZE * 2, UART_BUF_SIZE * 2, 0, NULL);
	uart_flush_input(UART_NUM_0);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	retres = setsmqtt(0);
	if (retres) retres = setstha();
	if (retres) retres = hassdiscovery();
//
//
	floop = 1;
	retres = setsmqtt(1);
	snfnum = 3;
//state monitoring and command execution loop
	while (floop) {
    if ((f_lfwnrf) && (f_rfwnrf)) {
	char buff[96];
	char buf1[16];
	buff[0] = 0;
      if (UPLIDX < UPLBUF_SIZE) {
        char  i = 0;
        char  d = 0;
        char  s = 16;
        char  al = 0;
        char  ah = 0;
        al = (UPLIDX & 255);
        ah = ((UPLIDX / 256) & 255);
        s = s + ah; s = s + al;
	strcpy(buff,MQTT_BASE_TOPIC);
	strcat(buff,"/:10");
        if (ah < 16) strcat(buff,"0");
	itoa(ah,buf1,16);
	strcat(buff,buf1);
        if (al < 16) strcat(buff,"0");
	itoa(al,buf1,16);
	strcat(buff,buf1);
	strcat(buff,"00");
        while (i < 16) {
	d = UPLBUF[UPLIDX]; s = s + d;
        if (d < 16) strcat(buff,"0");
	itoa(d,buf1,16);
	strcat(buff,buf1);
	UPLIDX++; i++;
        }
        s = 256 - s;
        if (s < 16) strcat(buff,"0");
	itoa(s,buf1,16);
	strcat(buff,buf1);
	strcat(buff,"\r\n");
        uart_write_bytes(UART_NUM_0, buff, strlen(buff));
        f_tfwnrf = 500;
        f_rfwnrf = false;
        f_sfwnrf = true;
      }
      else {
	strcpy(buff,MQTT_BASE_TOPIC);
	strcat(buff,"/:00000001FF\r\n");
        uart_write_bytes(UART_NUM_0, buff, strlen(buff));
        f_lfwnrf = false;
        f_sfwnrf = true;
      }
    } else vTaskDelay(48 / portTICK_PERIOD_MS);




        // Read data from the UART
	if (rxlnread()) {
	rxlnparse();
	if (!mqtdel && MQTT_TOPIC[0] && MQTT_DATA[0] && mqttConnected) {
	esp_mqtt_client_publish(mqttclient, MQTT_TOPIC, MQTT_DATA, 0, 1, 1);
	} else if (!mqtdel && MQTT_TOPIC[0] && MQTT_DATA[0] && !mqttConnected) {
	char ttopic[128];
	strcpy(ttopic,MQTT_BASE_TOPIC);
	strcat(ttopic,"/nrfcmd mqtt 0\r\n");
	if (floop) uart_write_bytes(UART_NUM_0, ttopic, strlen(ttopic));
	}

	} else if (wmode == 1) {
	if (!t_tsonof) {
	snfnum++;
	snfnum = snfnum & 3;
	}
	if (f_snfst1 || (!t_tsonof && (snfnum == 0))) {
	if (NAMONOF1[0]) {
	t_tsonof = 2500;
	strcpy(MQTT_TOPIC,MQTT_BASE_TOPIC);
	strcat(MQTT_TOPIC,"/");
	strcat(MQTT_TOPIC,NAMONOF1);
	strcpy(bufferln1,MQTT_TOPIC);
	strcat(bufferln1," ");
	(lvout ^ gpio_get_level(RELAY1))? strcpy(MQTT_DATA,STRON):strcpy(MQTT_DATA,STROF);
	strcat(bufferln1,MQTT_DATA);
	if (!mqtdel && mqttConnected) {
	esp_mqtt_client_publish(mqttclient, MQTT_TOPIC, MQTT_DATA, 0, 1, 1);
	f_snfsnd1 = true;
	f_snfst1 = false;
	}
	}
	} else if (f_snfst2 || (!t_tsonof && (snfnum == 1))) {
	if (NAMONOF2[0]) {
	t_tsonof = 2500;
	strcpy(MQTT_TOPIC,MQTT_BASE_TOPIC);
	strcat(MQTT_TOPIC,"/");
	strcat(MQTT_TOPIC,NAMONOF2);
	strcpy(bufferln1,MQTT_TOPIC);
	strcat(bufferln1," ");
	(lvout ^ gpio_get_level(RELAY2))? strcpy(MQTT_DATA,STRON):strcpy(MQTT_DATA,STROF);
	strcat(bufferln1,MQTT_DATA);
	if (!mqtdel && mqttConnected) {
	esp_mqtt_client_publish(mqttclient, MQTT_TOPIC, MQTT_DATA, 0, 1, 1);
	f_snfsnd2 = true;
	f_snfst2 = false;
	}
	}
	} else if (f_snfst3 || (!t_tsonof && (snfnum == 2))) {
	if (NAMONOF3[0]) {
	t_tsonof = 2500;
	strcpy(MQTT_TOPIC,MQTT_BASE_TOPIC);
	strcat(MQTT_TOPIC,"/");
	strcat(MQTT_TOPIC,NAMONOF3);
	strcpy(bufferln1,MQTT_TOPIC);
	strcat(bufferln1," ");
	(lvout ^ gpio_get_level(RELAY3))? strcpy(MQTT_DATA,STRON):strcpy(MQTT_DATA,STROF);
	strcat(bufferln1,MQTT_DATA);
	if (!mqtdel && mqttConnected) {
	esp_mqtt_client_publish(mqttclient, MQTT_TOPIC, MQTT_DATA, 0, 1, 1);
	f_snfsnd3 = true;
	f_snfst3 = false;
	}
	}
	} else if (f_snfst4 || (!t_tsonof && (snfnum == 3))) {
	if (NAMONOF4[0]) {
	t_tsonof = 2500;
	strcpy(MQTT_TOPIC,MQTT_BASE_TOPIC);
	strcat(MQTT_TOPIC,"/");
	strcat(MQTT_TOPIC,NAMONOF4);
	strcpy(bufferln1,MQTT_TOPIC);
	strcat(bufferln1," ");
	(lvout ^ gpio_get_level(RELAY4))? strcpy(MQTT_DATA,STRON):strcpy(MQTT_DATA,STROF);
	strcat(bufferln1,MQTT_DATA);
	if (!mqtdel && mqttConnected) {
	esp_mqtt_client_publish(mqttclient, MQTT_TOPIC, MQTT_DATA, 0, 1, 1);
	f_snfsnd4 = true;
	f_snfst4 = false;
	}
	}
	}

	if (f_snfsnd1 && (t_tsonof < 2125)) f_snfsnd1 = false; 
	if (f_snfsnd2 && (t_tsonof < 2125)) f_snfsnd2 = false; 
	if (f_snfsnd3 && (t_tsonof < 2125)) f_snfsnd3 = false; 
	if (f_snfsnd4 && (t_tsonof < 2125)) f_snfsnd4 = false; 
	if (!f_snfdel && (t_tsonof < 2125)) f_snfdel = true; 
	} else if (wmode == 2) {
	if (f_snfst1 || !t_tsonof) {
	if (NAMONOF1[0]) {
	t_tsonof = 2500;
	strcpy(MQTT_TOPIC,MQTT_BASE_TOPIC);
	strcat(MQTT_TOPIC,"/");
	strcat(MQTT_TOPIC,NAMONOF1);
	strcpy(bufferln1,MQTT_TOPIC);
	strcat(bufferln1," ");
	(lvout ^ gpio_get_level(RELAY1) || tdelcover)? strcpy(MQTT_DATA,STRON):strcpy(MQTT_DATA,STROF);
//	(lvout ^ gpio_get_level(RELAY1))? strcpy(MQTT_DATA,STRON):strcpy(MQTT_DATA,STROF);
	strcat(bufferln1,MQTT_DATA);
	if (!mqtdel && mqttConnected) {
	esp_mqtt_client_publish(mqttclient, MQTT_TOPIC, MQTT_DATA, 0, 1, 1);
	f_snfsnd1 = true;
	f_snfst1 = false;
	strcpy(tbuff,MQTT_BASE_TOPIC);
	strcat(tbuff,"/");
	strcat(tbuff,NAMONOF1);
	strcat(tbuff,"/count");
	itoa(lockcount,tzbuff,10);
	esp_mqtt_client_publish(mqttclient, tbuff, tzbuff, 0, 1, 1);
	}
	}
	}


	} else if ((wmode == 3) && NAMONOF1[0]) {
	if ((lvout ^ gpio_get_level(RELAY1)) && (lvout ^ gpio_get_level(RELAY2))) {
	gpio_set_level(RELAY1, lvout);
	gpio_set_level(RELAY2, lvout);
	crcvpos = 254;
	prcvpos = 255;
	}

	if ((crcvpos != prcvpos) || !t_tsonof) {
	t_tsonof = 2500;
	strcpy(MQTT_TOPIC,MQTT_BASE_TOPIC);
	strcat(MQTT_TOPIC,"/");
	strcat(MQTT_TOPIC,NAMONOF1);
	strcat(MQTT_TOPIC,"/state");
	strcpy(bufferln1,MQTT_TOPIC);
	strcat(bufferln1," ");
	if (lvout ^ gpio_get_level(RELAY1)) strcpy(MQTT_DATA,"opening");
	else if (lvout ^ gpio_get_level(RELAY2)) strcpy(MQTT_DATA,"closing");
	else if (!crcvpos) strcpy(MQTT_DATA,"closed");
	else if (crcvpos == 100) strcpy(MQTT_DATA,"open");
	else strcpy(MQTT_DATA,"stopped");
	strcat(bufferln1,MQTT_DATA);
	if (!mqtdel && mqttConnected) {
	esp_mqtt_client_publish(mqttclient, MQTT_TOPIC, MQTT_DATA, 0, 1, 1);
	strcpy(tbuff,MQTT_BASE_TOPIC);
	strcat(tbuff,"/");
	strcat(tbuff,NAMONOF1);
	strcat(tbuff,"/position");
	if (!crcvpos) esp_mqtt_client_publish(mqttclient, tbuff, "0", 0, 1, 1);
	else if (crcvpos == 100) esp_mqtt_client_publish(mqttclient, tbuff, "100", 0, 1, 1);
	else esp_mqtt_client_publish(mqttclient, tbuff, "50", 0, 1, 1);
	prcvpos = crcvpos;
	}

	}

	}
	if ((!f_lfwnrf) && (tuptime - t_last > 400)) {
//	wifi_ap_record_t wifidata;
	if (esp_wifi_sta_get_ap_info(&wifidata) == 0) {
	char buft[16];
	char bufd[8];
	iRssiESP = wifidata.rssi;
	if  (!mqtdel && mqttConnected && (iprevRssiESP != iRssiESP)) {
	strcpy(buft,MQTT_BASE_TOPIC);
	strcat(buft,"/rssi");
        itoa(iRssiESP,bufd,10);
	esp_mqtt_client_publish(mqttclient, buft, bufd, 0, 1, 1);
	iprevRssiESP = iRssiESP;
	}	
	}
	t_last = tuptime;
	}

        vTaskDelay(2 / portTICK_PERIOD_MS);
	if (s_retry_num >= WIFI_MAXIMUM_RETRY) floop = 0;
}
	esp_restart();
}

//*** include ***
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//#include "freertos/semphr.h"
//#include "freertos/queue.h"


#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "lwip/apps/sntp.h"
#include "esp_ota_ops.h"
#include "esp_event_loop.h"
#include "driver/hw_timer.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/uart.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include <stdint.h>
#include <stddef.h>


//#include "protocol_examples_common.h"
#include "esp_http_server.h"
#include "mqtt_client.h"


#define TAG "espNRF"

uint8_t espnrfnum = 9;                      //espnrf number
uint8_t espnrfnumo = 9;                     //espnrf number
uint16_t mqtt_port  = 1883;
uint32_t NumWfConn;
uint32_t NumMqConn;
uint32_t MemErr;

esp_mqtt_client_handle_t mqttclient; 
bool mqttConnected =false;
int   iRssiESP = 0;
int   iprevRssiESP = 0;


//*** wifi ***


#define WIFI_MAXIMUM_RETRY  10
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define UART_BUF_SIZE 1024                      // uart buffer size

#define UPLBUF_SIZE 6144                        // Maximum size of upload fw buffer
#define BUFFER_SIZE 200                         // Maximum size of one serial line and json content
#define MAX_TOPIC_LEN 96                        // Maximum topic path size (at least lenght of MQTT_PUBLISH_TOPIC + 2 x MAX_DATA_LEN)
#define MAX_DATA_LEN 48                         // Maximum size for record name, record id, json field name and json field value

#define otabufsize 2048

char tESP8266Addr[16];                      //esp mac
char MQTT_BASE_TOPIC[8];                    // nrfx
char MQTT_CLIENT_NAME[16];                  //espNRFx
char MQTT_USER[16];                         // MQTT Server user
char MQTT_PASSWORD[20];                     // MQTT Server password
char MQTT_SERVER[16];                       // MQTT Server
char WIFI_SSID[33];                         // network SSID for ESP32 to connect to
char WIFI_PASSWORD[65];                     // password for the network above


char NAMONOF1[33];
char NAMONOF2[33];
char NAMONOF3[33];
char NAMONOF4[33];
char STRON[16];
char STROF[16];
//#define RELAY1 14
//#define RELAY2 15
//#define BUTTON1 13
//#define BUTTON2 12
uint8_t RELAY1;
uint8_t RELAY2;
uint8_t RELAY3;
uint8_t RELAY4;
uint8_t BUTTON1;
uint8_t BUTTON2;
uint8_t BUTTON3;
uint8_t BUTTON4;
uint8_t CDELAY;
uint8_t wmode  = 0;
uint8_t prwmode = 0;
uint8_t stcvpos = 255;
uint8_t crcvpos = 255;
uint8_t prcvpos = 255;

bool f_snfsnd1 = false;
bool f_snfsnd2 = false;
bool f_snfsnd3 = false;
bool f_snfsnd4 = false;
bool f_snfst1 = false;
bool f_snfst2 = false;
bool f_snfst3 = false;
bool f_snfst4 = false;
bool f_snfdel = false;
bool f_button1 = false;
bool f_button2 = false;
bool f_button3 = false;
bool f_button4 = false;
int  n_button1 = 0;
int  n_button2 = 0;
int  n_button3 = 0;
int  n_button4 = 0;
uint8_t snfnum   = 0;
int  t_tsonof = 0;      // sonof update timer

uint8_t NRF_RESET_PIN;                         // ESP pin connected to NRF24 reset pin - GPIO2 = D4
uint8_t FDHass = 0; 
uint8_t ftrufal = 0; 
//uint8_t cmdmutex = 0;  // command mutex
uint8_t  TimeZone = 0;
static uint32_t tuptime = 0;
static uint16_t tdelcover = 0;
static uint32_t t_last = 0;
static uint32_t lockcount = 0;
uint8_t mqtdel  = 0;
uint8_t lvout  = 0;

//bool f_update = false;
bool f_lfwnrf = false;  // loading fw flag
bool f_rfwnrf = true;   // waiting for response fw flag
bool f_ffwnrf = false;  // last load error flag
bool f_sfwnrf = false;  // last load error flag
int  f_tfwnrf = 0;      // update timer

// main input / output buffers
char UPLBUF [UPLBUF_SIZE+32];
char MQTT_TOPIC[MAX_TOPIC_LEN];
char MQTT_DATA [BUFFER_SIZE];
char bufferln [BUFFER_SIZE];
char bufferln1 [BUFFER_SIZE];
uint8_t bufferrx [BUFFER_SIZE];
int cptbufrx = 0;
int lenbufrx = 0;
int cptbufln = 0;
int UPLIDX = 0;
int floop = 0;



char cssDatasheet[] = ""     // CSS
                        ""  // some CSS from ESP Easy Mega
                        "<style>* {font-family: sans-serif; font-size: 12pt; margin: 0px; padding: 0px; box-sizing: border-box; }"
                        "h1 {font-size: 16pt; color: #07D; margin: 8px 0; font-weight: bold; }"
                        "h2 {font-size: 11pt; margin: 0px; padding: 0px; font-weight: normal;}"
//		        "h2 {font-size: 12pt; margin: 0 -4px; padding: 6px; background-color: #444; color: #FFF; font-weight: bold; }"
			"h3 {font-size: 12pt; margin: 16px 0px 0px 0px; padding: 4px; background-color: #EEE; color: #444; font-weight: bold; }"
//			"h3 {font-size: 12pt; margin: 16px -4px 0 -4px; padding: 4px; background-color: #EEE; color: #444; font-weight: bold; }"
			"h6 {font-size: 10pt; color: #07D; }pre, xmp, code, kbd, samp, tt{ font-family:monospace,monospace; font-size:1em; }.button {margin: 4px; padding: 4px 16px; background-color: #07D; color: #FFF; text-decoration: none; border-radius: 4px; border: none;}.button.link { }.button.link.wide {display: inline-block; width: 100%; text-align: center;}.button.link.red {background-color: red;}.button.help {padding: 2px 4px; border-style: solid; border-width: 1px; border-color: gray; border-radius: 50%; }.button:hover {background: #369; } "
                        "input, select, textarea {margin: 4px; padding: 4px 8px; border-radius: 4px; background-color: #eee; border-style: solid; border-width: 1px; border-color: gray;}"

			"input:hover {background-color: #ccc; }input.wide {max-width: 500px; width:80%; }input.widenumber {max-width: 500px; width:100px; }#selectwidth {max-width: 500px; width:80%; padding: 4px 8px;}select:hover {background-color: #ccc; }.container {display: block; padding-left: 35px; margin-left: 4px; margin-top: 0px; position: relative; cursor: pointer; font-size: 12pt; -webkit-user-select: none; -moz-user-select: none; -ms-user-select: none; user-select: none; }.container input {position: absolute; opacity: 0; cursor: pointer;  }.checkmark {position: absolute; top: 0; left: 0; height: 25px;  width: 25px;  background-color: #eee; border-style: solid; border-width: 1px; border-color: gray;  border-radius: 4px;}.container:hover input ~ .checkmark {background-color: #ccc; }.container input:checked ~ .checkmark { background-color: #07D; }.checkmark:after {content: ''; position: absolute; display: none; }.container input:checked ~ .checkmark:after {display: block; }.container .checkmark:after {left: 7px; top: 3px; width: 5px; height: 10px; border: solid white; border-width: 0 3px 3px 0; -webkit-transform: rotate(45deg); -ms-transform: rotate(45deg); transform: rotate(45deg); }.container2 {display: block; padding-left: 35px; margin-left: 9px; margin-bottom: 20px; position: relative; cursor: pointer; font-size: 12pt; -webkit-user-select: none; -moz-user-select: none; -ms-user-select: none; user-select: none; }.container2 input {position: absolute; opacity: 0; cursor: pointer;  }.dotmark {position: absolute; top: 0; left: 0; height: 26px;  width: 26px;  background-color: #eee; border-style: solid; border-width: 1px; border-color: gray; border-radius: 50%;}.container2:hover input ~ .dotmark {background-color: #ccc; }.container2 input:checked ~ .dotmark { background-color: #07D;}.dotmark:after {content: ''; position: absolute; display: none; }.container2 input:checked ~ .dotmark:after {display: block; }.container2 .dotmark:after {top: 8px; left: 8px; width: 8px; height: 8px; border-radius: 50%; background: white; }#toastmessage {visibility: hidden; min-width: 250px; margin-left: -125px; background-color: #07D;color: #fff;  text-align: center;  border-radius: 4px;  padding: 16px;  position: fixed;z-index: 1; left: 282px; bottom: 30%;  font-size: 17px;  border-style: solid; border-width: 1px; border-color: gray;}#toastmessage.show {visibility: visible; -webkit-animation: fadein 0.5s, fadeout 0.5s 2.5s; animation: fadein 0.5s, fadeout 0.5s 2.5s; }@-webkit-keyframes fadein {from {bottom: 20%; opacity: 0;} to {bottom: 30%; opacity: 0.9;} }@keyframes fadein {from {bottom: 20%; opacity: 0;} to {bottom: 30%; opacity: 0.9;} }@-webkit-keyframes fadeout {from {bottom: 30%; opacity: 0.9;} to {bottom: 0; opacity: 0;} }@keyframes fadeout {from {bottom: 30%; opacity: 0.9;} to {bottom: 0; opacity: 0;} }.level_0 { color: #F1F1F1; }.level_1 { color: #FCFF95; }.level_2 { color: #9DCEFE; }.level_3 { color: #A4FC79; }.level_4 { color: #F2AB39; }.level_9 { color: #FF5500; }.logviewer {  color: #F1F1F1; background-color: #272727;  font-family: 'Lucida Console', Monaco, monospace;  height:  530px; max-width: 1000px; width: 80%; padding: 4px 8px;  overflow: auto;   border-style: solid; border-color: gray; }textarea {max-width: 1000px; width:80%; padding: 4px 8px; font-family: 'Lucida Console', Monaco, monospace; }textarea:hover {background-color: #ccc; } "
                        "table.normal th {padding: 6px; background-color: #444; color: #FFF; border-color: #888; font-weight: bold; } "
                        "table.normal td {padding: 4px; height: 30px;} "
                        "table.normal tr {padding: 4px; }table.normal {color: #000; width: 100%; min-width: 420px; border-collapse: collapse; } "

//                        "table.multirow td {text-align: left; font-size:0.9em;} h3 {margin: 16px 0px 0px 0px;} "
//                        "table.condensed td {padding: 0px 20px 0px 5px; height: 1em;} "

			"table.condensed tr {padding: 0px; }table.condensed {padding: 0px;border-left:1px solid #EEE;} "

			"table.multirow th {padding: 6px; background-color: #444; color: #FFF; border-color: #888; font-weight: bold; } "
			"table.multirow td {padding: 4px; text-align: left; font-size:0.9em; height: 30px;} "
			"table.multirow tr {padding: 4px; }table.multirow tr:nth-child(even){background-color: #DEE6FF; }table.multirow {color: #000; width: 100%; min-width: 420px; border-collapse: collapse; }tr.highlight "
                        "td {background-color: #F8F8F8;  }.note {color: #444; font-style: italic; }.headermenu "
//			"td {background-color: #dbff0075;}.note {color: #444; font-style: italic; }.headermenu "

                        "{position: fixed; top: 0; left: 0; right: 0; height: 90px; padding: 8px 12px; background-color: #F8F8F8; border-bottom: 1px solid #DDD; z-index: 1;}.apheader "
                        "{padding: 8px 12px; background-color: #F8F8F8;}.bodymenu {margin-top: 96px;}.menubar "
                        "{position: inherit; top: 55px; }.menu "
                        "{float: left; padding: 4px 16px 8px 16px; color: #444; white-space: nowrap; border: solid transparent; border-width: 4px 1px 1px; border-radius: 4px 4px 0 0; text-decoration: none; }.menu.active "
                        "{color: #000; background-color: #FFF; border-color: #07D #DDD #FFF; }.menu:hover "
                        "{color: #000; background: #DEF; }.menu_button "

			"{display: none;}.on {color: green; }.off {color: red; }.div_l {float: left; }.div_r {float: right; margin: 2px; padding: 1px 10px; border-radius: 4px; background-color: #080; color: white; }.div_br {clear: both; }.alert {padding: 20px; background-color: #f44336; color: white; margin-bottom: 15px; }.warning {padding: 20px; background-color: #ffca17; color: white; margin-bottom: 15px; }.closebtn {margin-left: 15px; color: white; font-weight: bold; float: right; font-size: 22px; line-height: 20px; cursor: pointer; transition: 0.3s; }.closebtn:hover {color: black; }"
                        "section{overflow-x: auto; width: 100%; }@media screen and (max-width: 960px) {span.showmenulabel { display: none; }.menu { max-width: 11vw; max-width: 48px; } "
                        "</style>";
//update form
const char* espserverIndex = "<form method='POST' action='/loadingesp' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
const char* nrfserverIndex = "<form method='POST' action='/loadingnrf' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Load'></form>";

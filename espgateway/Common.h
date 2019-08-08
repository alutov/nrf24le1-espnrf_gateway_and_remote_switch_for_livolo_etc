#ifndef H_COMMON
#define H_COMMON

#include <SoftwareSerial.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
//#include <ESP8266mDNS.h>
//#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>         // XXX
#include <ESP8266HTTPUpdateServer.h>  // XXX


/*********************************************************************************
 * Global parameters
/*********************************************************************************/

//#define WIFI_SSID "Lutov_A_N_2"                 // network SSID for ESP8266 to connect to
//#define WIFI_SSID "Lutov_A"                     // network SSID for ESP8266 to connect to
//#define WIFI_PASSWORD "LAN_22121967"           	// password for the network above
#define DEF_WIFI "espnrfwifi"                     // network SSID for ESP8266 to connect to

//#define MQTT_SERVER "192.168.1.10"              // MQTT Server
#define MQTT_PORT 1883                          // MQTT server port
//#define MQTT_USER "esp"                       	// MQTT Server user
//#define MQTT_PASSWORD "esp"                    	// MQTT Server password
//#define MQTT_RFLINK_CLIENT_NAME "espNRF4"       // Client name sent to MQTT server (also used for title, hostname and OTA)

#define NRF_RESET_PIN 2                        // ESP pin connected to MEGA reset pin - GPIO2 = D4
//#define MQTT_PUBLISH_TOPIC "nrf4/"             // MQTT topic to publish to
//#define MQTT_RFLINK_ORDER_TOPIC "nrf4/#"       // MQTT topic to listen to for rflink order
//#define MQTT_WILL_TOPIC "nrf4/online"          // MQTT last will topic
#define MQTT_WILL_ONLINE "1"                    // MQTT last will topic value online
#define MQTT_WILL_OFFLINE "0"                   // MQTT last will topic value online

//#define MQTT_DEBUG 0                           // XXX replaced by a variable to enable/disable
//#define MQTT_DEBUG_TOPIC "nrf4/debug"           // MQTT debug topic to publish raw data from RFLink
//#define MQTT_UPTIME_TOPIC "nrf4/uptime"         // MQTT topic for uptime

#define UPLBUF_SIZE 6144                        // Maximum size of upload fw buffer
#define BUFFER_SIZE 200                         // Maximum size of one serial line and json content
#define MAX_PRFX_LEN 8                          // Maximum size for prefix
#define MAX_DATA_LEN 48                         // Maximum size for record name, record id, json field name and json field value
#define MAX_TOPIC_LEN 96                        // Maximum topic path size (at least lenght of MQTT_PUBLISH_TOPIC + 2 x MAX_DATA_LEN)
#define MQTT_RFLINK_CLIENT_NAME_LEN 10          //"espNRF1" Client name sent to MQTT server (also used for title, hostname and OTA)
#define MQTT_PUBLISH_TOPIC_LEN 8                // "nrf1/" MQTT topic to publish to
#define MQTT_RFLINK_ORDER_TOPIC_LEN 8           // "nrf1/#" MQTT topic to listen to for nrf order
#define MQTT_WILL_TOPIC_LEN 16                  // "nrf1/online" MQTT last will topic
#define MQTT_DEBUG_TOPIC_LEN 16                 // "nrf1/debug" MQTT debug topic to publish raw data from nrf
#define MQTT_UPTIME_TOPIC_LEN 16                // "nrf1/uptime" MQTT topic for uptime
#define MQTT_USER_LEN 16                       	// "esp" MQTT Server user
#define MQTT_PASSWORD_LEN 16                   	// "esp" MQTT Server password
#define MQTT_SERVER_LEN 16          		// "192.168.1.10" MQTT Server
#define WIFI_SSID_LEN 16                  	// "Lutov_A_N_2" network SSID for ESP8266 to connect to
#define WIFI_PASSWORD_LEN 32           		// "LAN_22121967" password for the network above

/*********************************************************************************
 * functions defined in scketch
/*********************************************************************************/
void callback(char* topic, byte* payload, unsigned int length);
void buildMqttTopic(char *name, char *ID);
void printToSerial();

#endif

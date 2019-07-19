#include "Common.h"
#include "ArduinoJson.h"
#include "Rflink.h"
#include <Ticker.h>  //Ticker Library

Ticker blinker;

struct USER_ID_STRUCT {
  char id[MAX_DATA_LEN];        // ID
  long publish_interval;        // interval to publish (ms) if data did not change
  char description[60];         // description
};

struct USER_CMD_STRUCT {        // Preconfigured commands to show on web interface
  char text[MAX_DATA_LEN];      // button text
  char command[BUFFER_SIZE];    // command to send
};

/*********************************************************************************
 * Parameters for IDs filtering and serial comm ; see also parameters in Common.h
/*********************************************************************************/

#define USER_ID_NUMBER 0             // *MUST* be equal to USER_IDs number of lines OR set to 0 to publish all IDs with no condition


const USER_ID_STRUCT USER_IDs[] = {    // Configure IDs that will be forwared to MQTT server, interval time to force publish if data did not change (in ms), and description
  { "0001"    , 10 * 1000 * 60 , "Auriol V3 - Thermom&egrave;tre piscine (ID forc&eacute;e)"  },
//  { "2A1C"    , 10 * 1000 * 60 , "Oregon Rain2 - Pluie"                                       },
};

const USER_CMD_STRUCT USER_CMDs[] = {    // Configure commands to show on web interface
  { "Get"        ,  "get 1 1"                               },
  { "Add"        ,  "add 1 1 test"                          },
  { "Del"        ,  "del 1 1"                               },
  { "Ldfw"       ,  "ldfw 0"                                },
};

// Serial configuration

  SoftwareSerial softSerial(4, 0, false,BUFFER_SIZE+2); // software RX from GPIO4/D2 pin (unused pin on ESP01), software TX to RFLink on GPIO5/D7 pin (unused pin on ESP01)
  auto& debugSerialTX = softSerial;                     // debugSerialTX is to show information - use Serial to write on hardware serial (ESP TX pin) 
  auto& rflinkSerialRX = Serial;                        // rflinkSerialRX is for data from RFLink - use softSerial to listen on software serial, use Serial to listen on hardware serial (ESP RX pin)
  auto& rflinkSerialTX = Serial;                        // rflinkSerialTX is for data to RFLink - use softSerial (GPIO2/D4) to write on software serial, use Serial to write on hardware serial (ESP TX pin)

long resetMegaInterval = 0 * 1000 * 60;					// reset Mega if no data is received during more than x min - 0 to disable

/*********************************************************************************
 * Global Variables
/*********************************************************************************/

//#define SERIAL_DEBUG			// comment to disable debug functions
#if defined(SERIAL_DEBUG) /** Serial debug functions */
	//#define DEBUG_BEGIN(x)   	debugSerialTX.begin(x)
	#define DEBUG_PRINT(x)   	debugSerialTX.print(x)
	#define DEBUG_PRINTF(x,y)  	debugSerialTX.printf(x,y)
	#define DEBUG_PRINTLN(x) 	debugSerialTX.println(x)
	#define DEBUG_WRITE(x,y)   	debugSerialTX.write(x,y)
#else
	//#define DEBUG_BEGIN(x)   	{}
	#define DEBUG_PRINT(x)   	{}
	#define DEBUG_PRINTF(x,y)  	{}
	#define DEBUG_PRINTLN(x) 	{}
	#define DEBUG_WRITE(x, y) 	{}
#endif
  
bool MQTT_DEBUG = 0;		// debug variable to publish received data on MQTT debug topic ; default is disabled, can be enabled from web interface
bool f_lfwnrf = false;  // loading fw flag
bool f_rfwnrf = true;   // waiting for response fw flag
bool f_ffwnrf = false;  // last load error flag
bool f_sfwnrf = false;  // last load error flag
int  f_tfwnrf = 0;      // update timer
int  t_mqtcon = 0;      // mqtt connection attempt delay
long lastUptime = - 5 * 1000 * 60;			// timer to publish uptime on MQTT server ; sufficient negative value forces update at startup
long uptimeInterval = 5 * 1000 * 60;		// publish uptime every 5 min
long now;
long lastReceived = millis();     			// store last received (millis)
long UPLIDX = 0;
struct USER_ID_STRUCT_ALL {
  char id[8];              	// ID
  long publish_interval;  	// interval to publish (ms) if data did not change
  char description[60];    	// description
  char json[BUFFER_SIZE]; 	// store last json message
  long last_published;    	// store last publish (millis)
  long last_received;     	// store last received (millis)
};

USER_ID_STRUCT_ALL matrix[USER_ID_NUMBER];

// main input / output buffers
char UPLBUF [UPLBUF_SIZE];
char BUFFER [BUFFER_SIZE];
char BUFFER_DEBUG [5 + BUFFER_SIZE + 5 + MAX_DATA_LEN + 5 + MAX_DATA_LEN + 5 + MAX_TOPIC_LEN + 5 + BUFFER_SIZE + 5]; // It may be necessary to increase MQTT_MAX_PACKET_SIZE in PubSubClient.h
char JSON   [BUFFER_SIZE];
char JSON_DEBUG   [BUFFER_SIZE];

// message builder buffers
char MQTT_NAME[MAX_DATA_LEN];
char MQTT_PRFX[MAX_PRFX_LEN];
char MQTT_ID  [MAX_DATA_LEN];
char MQTT_TOPIC[MAX_TOPIC_LEN];
//char FIELD_BUF[MAX_DATA_LEN];

// Serial iterator counter
int CPT;

WiFiClient espClient;
PubSubClient MQTTClient(MQTT_SERVER, MQTT_PORT, callback, espClient);

ESP8266WebServer httpserver(80);                               // Create a webserver object that listens for HTTP request on port 80
ESP8266HTTPUpdateServer httpUpdater;                           // Firmware webupdate

void setup_wifi() {
        delay(10);
        DEBUG_PRINT(F("Connecting to "));
        DEBUG_PRINT(WIFI_SSID); DEBUG_PRINTLN(F(" ..."));
        WiFi.hostname(MQTT_RFLINK_CLIENT_NAME);
		WiFi.mode(WIFI_STA); 											// Act as wifi_client only, defaults to act as both a wifi_client and an access-point.
        WiFi.begin(WIFI_SSID,WIFI_PASSWORD);                            // Connect to the network
        int i = 0;
        while (WiFi.status() != WL_CONNECTED) {                         // Wait for the Wi-Fi to connect
          delay(1000);
          DEBUG_PRINT(++i); DEBUG_PRINT(' ');
        }
        DEBUG_PRINTLN('\n');
        DEBUG_PRINTLN(F("WiFi connected"));
        DEBUG_PRINT("IP address:\t ");
        DEBUG_PRINTLN(WiFi.localIP());
}

/**
 * callback to handle rflink order received from MQTT subscribtion
 */
 
void callback(char* topic, byte* payload, unsigned int len) {
        rflinkSerialTX.write(topic);
        rflinkSerialTX.write(" ");
        rflinkSerialTX.write(payload, len);
        rflinkSerialTX.print(F("\r\n"));
        DEBUG_PRINTLN(F("=== MQTT command ==="));
        DEBUG_PRINT(topic);
        DEBUG_PRINT(F(" = "));
        DEBUG_WRITE(payload, len);
        DEBUG_PRINT(F("\r\n"));
}

/**
 * build MQTT topic name to pubish to using parsed NAME and ID from rflink message
 */
 
void buildMqttTopic(char *name, char *ID) {
        MQTT_TOPIC[0] = '\0';
        strcat(MQTT_TOPIC,MQTT_PUBLISH_TOPIC);
        strcat(MQTT_TOPIC,"/");
        strcat(MQTT_TOPIC,MQTT_NAME);
}

/**
 * send formated message to serial
 */
 
void printToSerial() {
        DEBUG_PRINTLN();
        DEBUG_PRINTLN(F("=== RFLink packet ==="));
        DEBUG_PRINT(F("Raw data = ")); DEBUG_PRINT(BUFFER);
        DEBUG_PRINT(F("MQTT topic = "));
        DEBUG_PRINT(MQTT_TOPIC);
        DEBUG_PRINT(F("/ => "));
        DEBUG_PRINTLN(JSON);
        DEBUG_PRINTLN();
}

/**
 * try to connect to MQTT Server
 */
 
boolean MqttConnect() {

        // connect to Mqtt server and subcribe to order topic
        if (MQTTClient.connect(MQTT_RFLINK_CLIENT_NAME, MQTT_USER, MQTT_PASSWORD, MQTT_WILL_TOPIC, 0, 1, MQTT_WILL_OFFLINE)) { // XXX
                MQTTClient.subscribe(MQTT_RFLINK_ORDER_TOPIC);
                MQTTClient.publish(MQTT_WILL_TOPIC,MQTT_WILL_ONLINE,1);   // XXX once connected, update status of will topic
        }

        // report mqtt connection status
        DEBUG_PRINT(F("MQTT connection state : "));
        DEBUG_PRINTLN(MQTTClient.state());
        return MQTTClient.connected();
}

/**
 * OTA
 */
 
void SetupOTA() {

  // ArduinoOTA.setPort(8266);                                      // Port defaults to 8266
  ArduinoOTA.setHostname(MQTT_RFLINK_CLIENT_NAME);                  // Hostname defaults to esp8266-[ChipID]
  
  ArduinoOTA.onStart([]() {
    DEBUG_PRINTLN(F("Start OTA"));
  });

  ArduinoOTA.onEnd([]() {
    DEBUG_PRINTLN(F("End OTA"));
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DEBUG_PRINTF("Progress: %u%%\n", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    DEBUG_PRINTF("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) DEBUG_PRINTLN(F("Auth Failed"))
    else if (error == OTA_BEGIN_ERROR) DEBUG_PRINTLN(F("Begin Failed"))
    else if (error == OTA_CONNECT_ERROR) DEBUG_PRINTLN(F("Connect Failed"))
    else if (error == OTA_RECEIVE_ERROR) DEBUG_PRINTLN(F("Receive Failed"))
    else if (error == OTA_END_ERROR) DEBUG_PRINTLN(F("End Failed"))
  });

};
 
/**
 * HTTP server
 */
  
char cssEspEasy[] = ""     // CSS
	""     // from ESP Easy Mega release 20180914
	"* {font-family: sans-serif; font-size: 12pt; margin: 0px; padding: 0px; box-sizing: border-box; }h1 {font-size: 16pt; color: #07D; margin: 8px 0; font-weight: bold; }h2 {font-size: 12pt; margin: 0 -4px; padding: 6px; background-color: #444; color: #FFF; font-weight: bold; }h3 {font-size: 12pt; margin: 16px -4px 0 -4px; padding: 4px; background-color: #EEE; color: #444; font-weight: bold; }h6 {font-size: 10pt; color: #07D; }pre, xmp, code, kbd, samp, tt{ font-family:monospace,monospace; font-size:1em; }.button {margin: 4px; padding: 4px 16px; background-color: #07D; color: #FFF; text-decoration: none; border-radius: 4px; border: none;}.button.link { }.button.link.wide {display: inline-block; width: 100%; text-align: center;}.button.link.red {background-color: red;}.button.help {padding: 2px 4px; border-style: solid; border-width: 1px; border-color: gray; border-radius: 50%; }.button:hover {background: #369; }input, select, textarea {margin: 4px; padding: 4px 8px; border-radius: 4px; background-color: #eee; border-style: solid; border-width: 1px; border-color: gray;}input:hover {background-color: #ccc; }input.wide {max-width: 500px; width:80%; }input.widenumber {max-width: 500px; width:100px; }#selectwidth {max-width: 500px; width:80%; padding: 4px 8px;}select:hover {background-color: #ccc; }.container {display: block; padding-left: 35px; margin-left: 4px; margin-top: 0px; position: relative; cursor: pointer; font-size: 12pt; -webkit-user-select: none; -moz-user-select: none; -ms-user-select: none; user-select: none; }.container input {position: absolute; opacity: 0; cursor: pointer;  }.checkmark {position: absolute; top: 0; left: 0; height: 25px;  width: 25px;  background-color: #eee; border-style: solid; border-width: 1px; border-color: gray;  border-radius: 4px;}.container:hover input ~ .checkmark {background-color: #ccc; }.container input:checked ~ .checkmark { background-color: #07D; }.checkmark:after {content: ''; position: absolute; display: none; }.container input:checked ~ .checkmark:after {display: block; }.container .checkmark:after {left: 7px; top: 3px; width: 5px; height: 10px; border: solid white; border-width: 0 3px 3px 0; -webkit-transform: rotate(45deg); -ms-transform: rotate(45deg); transform: rotate(45deg); }.container2 {display: block; padding-left: 35px; margin-left: 9px; margin-bottom: 20px; position: relative; cursor: pointer; font-size: 12pt; -webkit-user-select: none; -moz-user-select: none; -ms-user-select: none; user-select: none; }.container2 input {position: absolute; opacity: 0; cursor: pointer;  }.dotmark {position: absolute; top: 0; left: 0; height: 26px;  width: 26px;  background-color: #eee; border-style: solid; border-width: 1px; border-color: gray; border-radius: 50%;}.container2:hover input ~ .dotmark {background-color: #ccc; }.container2 input:checked ~ .dotmark { background-color: #07D;}.dotmark:after {content: ''; position: absolute; display: none; }.container2 input:checked ~ .dotmark:after {display: block; }.container2 .dotmark:after {top: 8px; left: 8px; width: 8px; height: 8px; border-radius: 50%; background: white; }#toastmessage {visibility: hidden; min-width: 250px; margin-left: -125px; background-color: #07D;color: #fff;  text-align: center;  border-radius: 4px;  padding: 16px;  position: fixed;z-index: 1; left: 282px; bottom: 30%;  font-size: 17px;  border-style: solid; border-width: 1px; border-color: gray;}#toastmessage.show {visibility: visible; -webkit-animation: fadein 0.5s, fadeout 0.5s 2.5s; animation: fadein 0.5s, fadeout 0.5s 2.5s; }@-webkit-keyframes fadein {from {bottom: 20%; opacity: 0;} to {bottom: 30%; opacity: 0.9;} }@keyframes fadein {from {bottom: 20%; opacity: 0;} to {bottom: 30%; opacity: 0.9;} }@-webkit-keyframes fadeout {from {bottom: 30%; opacity: 0.9;} to {bottom: 0; opacity: 0;} }@keyframes fadeout {from {bottom: 30%; opacity: 0.9;} to {bottom: 0; opacity: 0;} }.level_0 { color: #F1F1F1; }.level_1 { color: #FCFF95; }.level_2 { color: #9DCEFE; }.level_3 { color: #A4FC79; }.level_4 { color: #F2AB39; }.level_9 { color: #FF5500; }.logviewer {  color: #F1F1F1; background-color: #272727;  font-family: 'Lucida Console', Monaco, monospace;  height:  530px; max-width: 1000px; width: 80%; padding: 4px 8px;  overflow: auto;   border-style: solid; border-color: gray; }textarea {max-width: 1000px; width:80%; padding: 4px 8px; font-family: 'Lucida Console', Monaco, monospace; }textarea:hover {background-color: #ccc; }table.normal th {padding: 6px; background-color: #444; color: #FFF; border-color: #888; font-weight: bold; }table.normal td {padding: 4px; height: 30px;}table.normal tr {padding: 4px; }table.normal {color: #000; width: 100%; min-width: 420px; border-collapse: collapse; }table.multirow th {padding: 6px; background-color: #444; color: #FFF; border-color: #888; font-weight: bold; }table.multirow td {padding: 4px; text-align: center;  height: 30px;}table.multirow tr {padding: 4px; }table.multirow tr:nth-child(even){background-color: #DEE6FF; }table.multirow {color: #000; width: 100%; min-width: 420px; border-collapse: collapse; }tr.highlight td { background-color: #dbff0075; }.note {color: #444; font-style: italic; }.headermenu {position: fixed; top: 0; left: 0; right: 0; height: 90px; padding: 8px 12px; background-color: #F8F8F8; border-bottom: 1px solid #DDD; z-index: 1;}.apheader {padding: 8px 12px; background-color: #F8F8F8;}.bodymenu {margin-top: 96px;}.menubar {position: inherit; top: 55px; }.menu {float: left; padding: 4px 16px 8px 16px; color: #444; white-space: nowrap; border: solid transparent; border-width: 4px 1px 1px; border-radius: 4px 4px 0 0; text-decoration: none; }.menu.active {color: #000; background-color: #FFF; border-color: #07D #DDD #FFF; }.menu:hover {color: #000; background: #DEF; }.menu_button {display: none;}.on {color: green; }.off {color: red; }.div_l {float: left; }.div_r {float: right; margin: 2px; padding: 1px 10px; border-radius: 4px; background-color: #080; color: white; }.div_br {clear: both; }.alert {padding: 20px; background-color: #f44336; color: white; margin-bottom: 15px; }.warning {padding: 20px; background-color: #ffca17; color: white; margin-bottom: 15px; }.closebtn {margin-left: 15px; color: white; font-weight: bold; float: right; font-size: 22px; line-height: 20px; cursor: pointer; transition: 0.3s; }.closebtn:hover {color: black; }section{overflow-x: auto; width: 100%; }@media screen and (max-width: 960px) {span.showmenulabel { display: none; }.menu { max-width: 11vw; max-width: 48px; }\r\n";
  
char cssDatasheet[] = ""     // CSS
	""	// force some changes in CSS compared to ESP Easy Mega
	"<style>table.multirow td {text-align: left; font-size:0.9em;} h3 {margin: 16px 0px 0px 0px;}\r\n"
	"table.condensed td {padding: 0px 20px 0px 5px; height: 1em;}table.condensed tr {padding: 0px; }table.condensed {padding: 0px;border-left:1px solid #EEE;}\r\n"
	"</style>\r\n";
	
void ConfigHTTPserver() {

  httpserver.on("/esp.css",[](){       // CSS EspEasy
  
    String cssMessage = String(cssEspEasy);
	httpserver.sendHeader("Access-Control-Max-Age", "86400");
	httpserver.send(200, "text/css", cssMessage);
	
  });
	
  httpserver.on("/",[](){       // Main, page d'accueil
  
    String htmlMessage = "<!DOCTYPE html>\r\n<html>\r\n";

    // Head
    htmlMessage += "<head><title>" + String(MQTT_RFLINK_CLIENT_NAME) + "</title>\r\n";
    htmlMessage += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>\r\n";
	htmlMessage += "<link rel=\"stylesheet\" type=\"text/css\" href=\"esp.css\">\r\n";
	htmlMessage += String(cssDatasheet);	// CSS
    htmlMessage += "</head>\r\n";

    // Body
    htmlMessage += "<body class='bodymenu'>\r\n";

    // Header + menu
    htmlMessage += "<header class='headermenu'>";
    htmlMessage += "<h1>" + String(MQTT_RFLINK_CLIENT_NAME) + "</h1>\r\n";
    htmlMessage += "<div class='menubar'><a class='menu active' href='.'>&#8962;<span class='showmenulabel'>Main</span></a>\r\n";
    htmlMessage += "<a class='menu' href='livedata'>&#10740;<span class='showmenulabel'>Live Data</span></a>\r\n";
    htmlMessage += "<a id='resetmega' class='menu' href='reset-nrf'>&#128204;<span class='showmenulabel'>Reset NRF</span></a>\r\n";
    htmlMessage += "<script>document.getElementById(\"resetmega\").onclick = function(e) {\r\n"
		"var wnd = window.open(\"reset-nrf\")\r\n"
		"wnd.close();e.preventDefault();};</script>";
    htmlMessage += "<a class='menu' id='reboot' href='reboot'>&#128268;<span class='showmenulabel'>Reboot ESP</span></a>\r\n";
    htmlMessage += "<script>document.getElementById(\"reboot\").onclick = function(e) {\r\n"
		"var wnd = window.open(\"reboot\")\r\n"
		"wnd.close();e.preventDefault();"
		"setTimeout(function(){ location.reload(); }, 5000);"
		"};"
		"</script>";
    htmlMessage += (!MQTT_DEBUG)? "<a class='menu' href='enable-debug'>&#128172;<span class='showmenulabel'>Enable MQTT debug topic</span></a>\r\n":"<a class='menu' href='disable-debug'>&#128172;<span class='showmenulabel'>Disable MQTT debug topic</span></a>\r\n";
    htmlMessage += "<a class='menu' href='lfwnrf'>&#128295;<span class='showmenulabel'>Load NRF firmware</span></a>\r\n";
    htmlMessage += "<a class='menu' href='update'>&#128295;<span class='showmenulabel'>Load ESP firmware</span></a></div>\r\n";
    htmlMessage += "</header>\r\n";
    
    // Live data
    htmlMessage += "<h3>NRF Live Data *</h3>\r\n";

    htmlMessage += "<input type=\"button\" value =\"Pause\" onclick=\"stopUpdate();\" />";                                                                // Pause
    htmlMessage += "<input type=\"button\" value =\"Restart\" onclick=\"restartUpdate();\" />";                                                           // Restart
    htmlMessage += "<input type=\"button\" value =\"Refresh\" onclick=\"window.location.reload(true);\" />\r\n";                                          // Refresh
    htmlMessage += "<input type=\"text\" id=\"mySearch\" onkeyup=\"filterLines()\" placeholder=\"Search for...\" title=\"Type in a name\"><br />\r\n";    // Search
    
    htmlMessage += "<table id=\"liveData\" class='multirow';>\r\n";                                                                                       // Table of x lines
    htmlMessage += "<tr class=\"header\"><th style='text-align:left;'>Raw Data</th><th style='text-align:left;'> MQTT Topic </th><th style='text-align:left;'> MQTT JSON </th></tr>\r\n";
    for (int i = 0; i < (5); i++){ // not too high to avoid overflow
      htmlMessage += "<tr id=\"data" + String(i) + "\"><td></td><td></td><td></td></tr>\r\n";
    } 
    htmlMessage += "</table>\r\n";
    
    htmlMessage += "<script>\r\n";                                                       // Script to filter lines
    htmlMessage += "function filterLines() {\r\n";
    htmlMessage += "  var input, filter, table, tr, td, i;\r\n";
    htmlMessage += "  input = document.getElementById(\"mySearch\");\r\n";
    htmlMessage += "  filter = input.value.toUpperCase();\r\n";
    htmlMessage += "  table = document.getElementById(\"liveData\");\r\n";
    htmlMessage += "  tr = table.getElementsByTagName(\"tr\");\r\n";
    htmlMessage += "  for (i = 0; i < tr.length; i++) {\r\n";
    htmlMessage += "    td = tr[i].getElementsByTagName(\"td\")[0];\r\n";
    htmlMessage += "    if (td) {\r\n";
    htmlMessage += "      if (td.innerHTML.toUpperCase().indexOf(filter) > -1) {\r\n";
    htmlMessage += "        tr[i].style.display = \"\";\r\n";
    htmlMessage += "      } else {\r\n";
    htmlMessage += "        tr[i].style.display = \"none\";\r\n";
    htmlMessage += "      }\r\n";
    htmlMessage += "    }       \r\n";
    htmlMessage += "  }\r\n";
    htmlMessage += "}\r\n";
    htmlMessage += "</script>\r\n";
    
    htmlMessage += "<script>\r\n";                                                        // Script to update data and move to next line
    htmlMessage += "var x = setInterval(function() {loadData(\"data.txt\",updateData)}, 500);\r\n";       // update every 500 ms
    htmlMessage += "function loadData(url, callback){\r\n";
    htmlMessage += "var xhttp = new XMLHttpRequest();\r\n";
    htmlMessage += "xhttp.onreadystatechange = function(){\r\n";
    htmlMessage += " if(this.readyState == 4 && this.status == 200){\r\n";
    htmlMessage += " callback.apply(xhttp);\r\n";
    htmlMessage += " }\r\n";
    htmlMessage += "};\r\n";
    htmlMessage += "xhttp.open(\"GET\", url, true);\r\n";
    htmlMessage += "xhttp.send();\r\n";
    htmlMessage += "}\r\n";
    htmlMessage += "var memorized_data;\r\n";
    htmlMessage += "function updateData(){\r\n";
    htmlMessage += "if (memorized_data != this.responseText) {\r\n";
    for (int i = (5 - 1); i > 0; i--){	// not too high to avoid overflow
      htmlMessage += "document.getElementById(\"data" + String(i) + "\").innerHTML = document.getElementById(\"data" + String (i-1) + "\").innerHTML;\r\n";
    }
    htmlMessage += "}\r\n";
    htmlMessage += "document.getElementById(\"data0\").innerHTML = this.responseText;\r\n";
    htmlMessage += "memorized_data = this.responseText;\r\n"; // memorize new data
    htmlMessage += "filterLines();\r\n";                      // apply filter from mySearch input
    htmlMessage += "}\r\n";
    htmlMessage += "function stopUpdate(){\r\n";
    htmlMessage += " clearInterval(x);\r\n";
    htmlMessage += "}\r\n";
    htmlMessage += "function restartUpdate(){\r\n";
    htmlMessage += " x = setInterval(function() {loadData(\"data.txt\",updateData)}, 500);\r\n";       // update every 500 ms
    htmlMessage += "}\r\n";
    htmlMessage += "</script>\r\n";
    htmlMessage += "<div class='note'>* see Live \"Data tab\" for more lines - web view may not catch all frames, MQTT debug is more accurate</div>\r\n";
    
    // Commands to RFLink
    htmlMessage += "<h3>Commands to NRF</h3><br />";
    htmlMessage += "<form action=\"/send\" id=\"form_command\" style=\"float: left;\"><input type=\"text\" id=\"command\" name=\"command\">";
    htmlMessage += "<input type=\"submit\" value=\"Send\"><a class='button help' href='http://www.rflink.nl/blog2/protref' target='_blank'>&#10068;</a></form>\r\n";
//	htmlMessage += "<script>function submitForm() {"
//		"var http = new XMLHttpRequest();"
//		"http.open(\"POST\", \"/send\", true);"
//		"http.setRequestHeader(\"Content-type\",\"application/x-www-form-urlencoded\");"
//		"var params = \"command=\" + document.getElementById('command').value;"
//		"http.send(params);"
//		"}</script>";
    for (int i = 0; i < (sizeof(USER_CMDs) / sizeof(USER_CMDs[0])); i++){      // User commands defined in USER_CMDs for quick access
      htmlMessage += "<a class='button link' style=\"float: left;\" href=\"javascript:{}\""; 
      htmlMessage += "onclick=\"document.getElementById('command').value = '" + String(USER_CMDs[i].command) + "';";
//      htmlMessage += "submitForm(); return false;\">" + String(USER_CMDs[i].text) + "</a>\r\n";  
      htmlMessage += "return false;\">" + String(USER_CMDs[i].text) + "</a>\r\n";  

	}
    htmlMessage += "<br style=\"clear: both;\"/>\r\n";

    // System Info
    htmlMessage += "<h3>System Info</h3>\r\n";
    htmlMessage += "<table class='normal'>\r\n";
    htmlMessage += "<tr><td style='min-width:150px;'>Version</td><td style='width:80%;'>2019.07.17</td></tr>\r\n";
    htmlMessage += "<tr><td>Uptime</td><td>" + String(uptime_string_exp()) + "</td></tr>\r\n";
    htmlMessage += "<tr><td>WiFi network</td><td>" + String(WiFi.SSID()) + " (" + WiFi.BSSIDstr() + ")</td></tr>\r\n";
    htmlMessage += "<tr><td>WiFi RSSI</td><td>" + String(WiFi.RSSI()) + " dB</td></tr>\r\n";
    htmlMessage += "<tr><td>IP address (MAC)</td><td>" + WiFi.localIP().toString() +" (" + String(WiFi.macAddress()) +")</td></tr>\r\n";
    //htmlMessage += "<tr><td>ESP pin to reset NRF</td><td>GPIO " + String(NRF_RESET_PIN) + "</td></tr>";
	  htmlMessage += "<tr><td>ESP pin to reset NRF</td><td>GPIO " + String(NRF_RESET_PIN);
  if (resetMegaInterval > 0) {
		htmlMessage += " - auto reset after " + String( int(float(resetMegaInterval) *0.001 /60)) + " min if no data received - last data " + time_string_exp(now - lastReceived) + " ago";
	}
	htmlMessage += "</td></tr>";
    htmlMessage += "<tr><td>MQTT server and port</td><td>" + String(MQTT_SERVER) + ":" + String(MQTT_PORT)+"</td></tr>\r\n";
    htmlMessage += "<tr><td>MQTT debug topic</td><td>";
    (MQTT_DEBUG)? htmlMessage += "<span style=\"font-weight:bold\">enabled</span>" : htmlMessage += "disabled";
    htmlMessage += "</td></tr>\r\n";
    htmlMessage += "<tr><td>MQTT connection state <a class='button help' href='https://pubsubclient.knolleary.net/api.html#state' target='_blank'>&#10068;</a></td><td>" + String(MQTTClient.state()) + "</td></tr>\r\n";
    htmlMessage += "<tr><td>MQTT topics</td><td><table class='condensed'>\r\n";
    htmlMessage += "<tr><td>publish (json)</td><td> " + String(MQTT_PUBLISH_TOPIC) + "/Name</td></tr>\r\n";
    htmlMessage += "<tr><td>commands to NRF</td><td> " + String(MQTT_PUBLISH_TOPIC) + "/nrfcmd</td></tr>\r\n";
    htmlMessage += "<tr><td>last will ( " + String(MQTT_WILL_ONLINE) + " / " + String(MQTT_WILL_OFFLINE) + " )</td><td> " + String(MQTT_WILL_TOPIC) + "</td></tr>\r\n";
    htmlMessage += "<tr><td>uptime (min, every " + String( int( float(uptimeInterval) *0.001 / 60) ) + ")</td><td> " + String(MQTT_UPTIME_TOPIC) + "</td></tr>\r\n";
    htmlMessage += "<tr><td>debug (data from NRF)</td><td> " + String(MQTT_DEBUG_TOPIC) + "</td></tr>\r\n";
    htmlMessage += "</table></td></tr>\r\n";
    //htmlMessage += "<tr><td>User specific</td><td>ID for protocol Auriol V3 is forced to 0001</td></tr>\r\n";
    htmlMessage += "</table><br />\r\n";

    // User ID
    htmlMessage += "<h3>User IDs configuration *</h3>\r\n";
    if (USER_ID_NUMBER > 0) {                                                                         // show list of user IDs
      int i;
      htmlMessage += "<table class='multirow'><tr><th>ID</th><th>Description</th><th>Interval</th><th>Received</th><th>Published</th><th style='text-align:left;'>Last MQTT JSON</th></tr>\r\n";
      for (i = 0; i < (USER_ID_NUMBER); i++){
        htmlMessage += "<tr><td style=\"text-align: center;\">" + String(matrix[i].id) + "</td>";
        htmlMessage += "<td style=\"text-align: left;\">" + String(matrix[i].description) + "</td>";
        htmlMessage += "<td style=\"text-align: center;\">" + String( int(float(matrix[i].publish_interval) *0.001 /60)) + " min</td>";
        if (matrix[i].last_received != 0) {
          //htmlMessage += "<td style=\"text-align: center;\">" + String( int( (float(now) - float(matrix[i].last_received)) *0.001 /60 ) ) + " min ago</td>";
		  htmlMessage += "<td style=\"text-align: center;\">" + time_string_exp(now - matrix[i].last_received) + "</td>";
        } else {
          htmlMessage += "<td style=\"text-align: center;\"></td>";
        }
        if (matrix[i].last_published != 0) {
          //htmlMessage += "<td style=\"text-align: center;\">" + String( int( (float(now) - float(matrix[i].last_published)) *0.001 /60 ) ) + " min ago</td>";
          htmlMessage += "<td style=\"text-align: center;\">" + time_string_exp(now - matrix[i].last_published) + "</td>";
        } else {
          htmlMessage += "<td style=\"text-align: center;\"></td>";
        }
        htmlMessage += "<td>" + String(matrix[i].json) + "</td></tr>\r\n";
      }
      htmlMessage += "</table>\r\n";
      htmlMessage += "<div class='note'>* only those IDs are published on MQTT server, and only if data changed or interval time is exceeded</div>\r\n";
    } else {
      htmlMessage += "<div class='note'>* all IDs are forwarded to MQTT server</div>\r\n";
    }
    //htmlMessage += "<span style=\"font-size: 0.9em\">Running for " + String( int(float(now) *0.001 /60)) + " minutes </span>\r\n";
    htmlMessage += "<span style=\"font-size: 0.9em\">Running for " + uptime_string_exp() + " </span>\r\n";
    htmlMessage += "<input type=\"button\" value =\"Refresh\" onclick=\"window.location.reload(true);\" />\r\n";
    htmlMessage += "<footer><br><h6>Powered by <a href='https://github.com/seb821' style='font-size: 15px; text-decoration: none'>github.com/seb821</a></h6></footer>\r\n";
    htmlMessage += "</body>\r\n</html>\r\n";
    httpserver.send(200, "text/html", htmlMessage);
  }); // server.on("/"...



  httpserver.on("/livedata",[](){       // 

    String htmlMessage = "<!DOCTYPE html>\r\n<html>\r\n";

    // Head
    htmlMessage += "<head><title>" + String(MQTT_RFLINK_CLIENT_NAME) + "</title>\r\n";
    htmlMessage += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>\r\n";
	htmlMessage += "<link rel=\"stylesheet\" type=\"text/css\" href=\"esp.css\">\r\n";
	htmlMessage += String(cssDatasheet);	// CSS
    htmlMessage += "</head>\r\n";

    // Body
    htmlMessage += "<body class='bodymenu'>\r\n";

    // Header + menu
    htmlMessage += "<header class='headermenu'>";
    htmlMessage += "<h1>" + String(MQTT_RFLINK_CLIENT_NAME) + "</h1>\r\n";
    htmlMessage += "<div class='menubar'><a class='menu' href='.'>&#8962;<span class='showmenulabel'>Main</span></a>\r\n";
    htmlMessage += "<a class='menu active' href='livedata'>&#10740;<span class='showmenulabel'>Live Data</span></a>\r\n";
    htmlMessage += "<a id='resetmega' class='menu' href='#'>&#128204;<span class='showmenulabel'>Reset NRF</span></a>\r\n";
    htmlMessage += "<script>document.getElementById(\"resetmega\").onclick = function(e) {\r\n"
		"var wnd = window.open(\"reset-nrf\")\r\n"
		"wnd.close();e.preventDefault();};</script>";
    htmlMessage += "<a class='menu' id='reboot' href='reboot'>&#128268;<span class='showmenulabel'>Reboot ESP</span></a>\r\n";
    htmlMessage += "<script>document.getElementById(\"reboot\").onclick = function(e) {\r\n"
		"var wnd = window.open(\"reboot\")\r\n"
		"wnd.close();e.preventDefault();"
		// onreload on main page // "setTimeout(function(){ location.reload(); }, 5000);" //
		"};"
		"</script>";
    htmlMessage += (!MQTT_DEBUG)? "<a class='menu' href='enable-debug'>&#128172;<span class='showmenulabel'>Enable MQTT debug topic</span></a>\r\n":"<a class='menu' href='disable-debug'>&#128172;<span class='showmenulabel'>Disable MQTT debug topic</span></a>\r\n";
    htmlMessage += "<a class='menu' href='lfwnrf'>&#128295;<span class='showmenulabel'>Load NRF firmware</span></a>\r\n";
    htmlMessage += "<a class='menu' href='update'>&#128295;<span class='showmenulabel'>Load ESP firmware</span></a></div>\r\n";
    htmlMessage += "</header>\r\n";
    
    // Live data
    htmlMessage += "<h3>NRF Live Data</h3>\r\n";

    htmlMessage += "<input type=\"button\" value =\"Pause\" onclick=\"stopUpdate();\" />";                                                                // Pause
    htmlMessage += "<input type=\"button\" value =\"Restart\" onclick=\"restartUpdate();\" />";                                                           // Restart
    htmlMessage += "<input type=\"button\" value =\"Refresh\" onclick=\"window.location.reload(true);\" />\r\n";                                          // Refresh                                                                                   // OK
    htmlMessage += "<input type=\"text\" id=\"mySearch\" onkeyup=\"filterLines()\" placeholder=\"Search for...\" title=\"Type in a name\"><br />\r\n";    // Search
    
    htmlMessage += "<table id=\"liveData\" class='multirow';>\r\n";                                                                                       // Table of x lines
    htmlMessage += "<tr class=\"header\"><th style='text-align:left;'> <a onclick='sortTable(0)'>Time</a></th><th style='text-align:left;'> <a onclick='sortTable(1)'>Raw Data</a> </th><th style='text-align:left;'> <a onclick='sortTable(2)'>MQTT Topic</a> </th><th style='text-align:left;'> <a onclick='sortTable(3)'>MQTT JSON</a> </th></tr>\r\n";
	htmlMessage += "<tr id=\"data0" "\"><td></td><td></td><td></td><td></td></tr>\r\n";
    htmlMessage += "</table>\r\n";
    
    htmlMessage += "<script>\r\n";                                                       // Script to filter lines
    htmlMessage += "function filterLines() {\r\n";
    htmlMessage += "  var input, filter, table, tr, td, i;\r\n";
    htmlMessage += "  input = document.getElementById(\"mySearch\");\r\n";
    htmlMessage += "  filter = input.value.toUpperCase();\r\n";
    htmlMessage += "  table = document.getElementById(\"liveData\");\r\n";
    htmlMessage += "  tr = table.getElementsByTagName(\"tr\");\r\n";
    htmlMessage += "  for (i = 0; i < tr.length; i++) {\r\n";
    htmlMessage += "    td = tr[i].getElementsByTagName(\"td\")[1];\r\n";
    htmlMessage += "    if (td) {\r\n";
    htmlMessage += "      if (td.innerHTML.toUpperCase().indexOf(filter) > -1) {\r\n";
    htmlMessage += "        tr[i].style.display = \"\";\r\n";
    htmlMessage += "      } else {\r\n";
    htmlMessage += "        tr[i].style.display = \"none\";\r\n";
    htmlMessage += "      }\r\n";
    htmlMessage += "    }       \r\n";
    htmlMessage += "  }\r\n";
    htmlMessage += "}\r\n";
    htmlMessage += "</script>\r\n";
    htmlMessage += "<script>\r\n";                                                        // Script to update data and move to next line
    htmlMessage += "var x = setInterval(function() {loadData(\"data.txt\",updateData)}, 250);\r\n";       // update every 250 ms
    htmlMessage += "function loadData(url, callback){\r\n";
    htmlMessage += "var xhttp = new XMLHttpRequest();\r\n";
    htmlMessage += "xhttp.onreadystatechange = function(){\r\n";
    htmlMessage += " if(this.readyState == 4 && this.status == 200){\r\n";
    htmlMessage += " callback.apply(xhttp);\r\n";
    htmlMessage += " }\r\n";
    htmlMessage += "};\r\n";
    htmlMessage += "xhttp.open(\"GET\", url, true);\r\n";
    htmlMessage += "xhttp.send();\r\n";
    htmlMessage += "}\r\n";
    htmlMessage += "var memorized_data;\r\n";
    htmlMessage += "function roll() {\r\n"
      "var table = document.getElementById('liveData');\r\n"
      "var rows = table.rows;\r\n"
      "var firstRow = rows[1];\r\n"
      "var clone = firstRow.cloneNode(true);\r\n"
	  "var target = rows[1];\r\n"
	  "var newElement = clone;\r\n"
	  "target.parentNode.insertBefore(newElement, target.nextSibling );\r\n"
	  "}\r\n";
    htmlMessage += "function updateData(){\r\n";
    htmlMessage += "if (memorized_data != this.responseText) {\r\n";
	htmlMessage += "roll();";
    htmlMessage += "var date = new Date;\r\n";
    htmlMessage += "h = date.getHours(); if(h<10) {h = '0'+h;}; m = date.getMinutes(); if(m<10) {m = '0'+m;}; s = date.getSeconds(); if(s<10) {s = '0'+s;}\r\n";
    htmlMessage += "document.getElementById('data0').innerHTML = '<td>' + h + ':' + m + ':' + s + '</td>' + this.responseText;\r\n";
    htmlMessage += "memorized_data = this.responseText;\r\n"; // memorize new data
    htmlMessage += "filterLines();\r\n";                      // apply filter from mySearch input
    htmlMessage += "}\r\n";
    htmlMessage += "}\r\n";
    htmlMessage += "function stopUpdate(){\r\n";
    htmlMessage += " clearInterval(x);\r\n";
    htmlMessage += "}\r\n";
    htmlMessage += "function restartUpdate(){\r\n";
    htmlMessage += " x = setInterval(function() {loadData(\"data.txt\",updateData)}, 250);\r\n";       // update every 250 ms
    htmlMessage += "}\r\n";
    htmlMessage += "</script>\r\n";
	htmlMessage += ""
		"<script>\r\n"
		"function sortTable(column) {\r\n"
		"  var table, rows, switching, i, x, y, shouldSwitch;\r\n"
		"  table = document.getElementById(\"liveData\");\r\n"
		"  switching = true;\r\n"
		"  while (switching) {\r\n"
		"	switching = false;\r\n"
		"	rows = table.rows;\r\n"
		"	for (i = 1; i < (rows.length - 1); i++) {\r\n"
		"	  shouldSwitch = false;\r\n"
		"	  x = rows[i].getElementsByTagName(\"TD\")[column];\r\n"
		"	  y = rows[i + 1].getElementsByTagName(\"TD\")[column];\r\n"
		"	  if (x.innerHTML.toLowerCase() < y.innerHTML.toLowerCase()) {\r\n"
		"		shouldSwitch = true;\r\n"
		"		break;\r\n"
		"	  }\r\n"
		"	}\r\n"
		"	if (shouldSwitch) {\r\n"
		"	  rows[i].parentNode.insertBefore(rows[i + 1], rows[i]);\r\n"
		"	  switching = true;\r\n"
		"	}\r\n"
		"  }\r\n"
		"}\r\n"
		"</script>\r\n";
    //htmlMessage += "<span style=\"font-size: 0.9em\">Running for " + String( int(float(now) *0.001 /60)) + " minutes </span>\r\n";
    htmlMessage += "<span style=\"font-size: 0.9em\">Running for " + uptime_string_exp() + " </span>\r\n";
    htmlMessage += "<input type=\"button\" value =\"Refresh\" onclick=\"window.location.reload(true);\" />\r\n";
    htmlMessage += "<footer><br><h6>Powered by <a href='https://github.com/seb821' style='font-size: 15px; text-decoration: none'>github.com/seb821</a></h6></footer>\r\n";
    htmlMessage += "</body>\r\n</html>\r\n";
    httpserver.send(200, "text/html", htmlMessage);
	
  }); // livedata
  
  httpserver.on("/reboot",[](){                                	// Reboot ESP
    DEBUG_PRINTLN("Rebooting device...");
    //httpserver.sendHeader("Location","/");                    // Add a header to respond with a new location for the browser to go to the home page again
    //httpserver.send(303);                                     // Send it back to the browser with an HTTP status 303 (See Other) to redirect
    httpserver.send(200, "text/html", "Rebooting...");
	delay(50);
    ESP.restart();
    //ESP.reset();
  });

  
  httpserver.on("/lfwnrf",[](){                              // Send FW to NRF24
    String htmlMessage = "<!DOCTYPE html>\r\n<html>\r\n";
    htmlMessage += "</head>\r\n";
    htmlMessage += "<form action='/lfwnrf1' enctype='multipart/form-data' method='POST'>";
    htmlMessage += "<input type='hidden' name ='MAX_FILE_SIZE' value='6000'>";
    htmlMessage += "<input type='file' name='myfile'>";
    htmlMessage += "<input type='submit' value='Upload'>";
    htmlMessage += "</form>";    
   
//    delay(50);
    httpserver.send(200, "text/html", htmlMessage);
  });
 httpserver.onFileUpload([]() {
      HTTPUpload& upload = httpserver.upload();
    if (upload.status == UPLOAD_FILE_START) {
    UPLIDX = 0;
    char d = 255;
    while (UPLIDX < UPLBUF_SIZE) {
    UPLBUF[UPLIDX] = d;
    UPLIDX++;
    }   
    UPLIDX = 0;
    }
    else if (upload.status == UPLOAD_FILE_WRITE) {
    int s = 0;
    while(s < upload.currentSize) {
    if (UPLIDX < UPLBUF_SIZE) {
    UPLBUF[UPLIDX] = upload.buf[s];
    }
                    s++; UPLIDX++;
    }   
    }
    else if (upload.status == UPLOAD_FILE_END) {
//    DEBUG_PRINT("At close: ");
//    DEBUG_PRINTLN(UPLIDX);

    if (UPLIDX < (UPLBUF_SIZE+1)) {
    UPLIDX = 0;
    f_lfwnrf = true;    // load fw variable
    f_rfwnrf = true;
    f_ffwnrf = false;
    f_sfwnrf = true;
    }
    
    }
      yield();
    });
  
  httpserver.on("/lfwnrf1",[](){
    String htmlMessage = "<!DOCTYPE html>\r\n<html>\r\n";
    // Head
    htmlMessage += "<head><title>" + String(MQTT_RFLINK_CLIENT_NAME) + "</title>\r\n";
    htmlMessage += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>\r\n";
    htmlMessage += "<link rel=\"stylesheet\" type=\"text/css\" href=\"esp.css\">\r\n";
    htmlMessage += String(cssDatasheet);  // CSS
//    delay(50);
    httpserver.sendHeader("Location","/");        // Add a header to respond with a new location for the browser to go to the home page again
    httpserver.send(303);                         // Send it back to the browser with an HTTP status 303 (See Other) to redirect
    });

  httpserver.on("/reset-nrf",[](){                             // Reset NRF24
    DEBUG_PRINTLN(F("Resetting NRF..."));
    pinMode(NRF_RESET_PIN, OUTPUT);
    digitalWrite(NRF_RESET_PIN,false);                         // Change the state of pin to ground
    delay(1000); 
    digitalWrite(NRF_RESET_PIN,true);                          // Change the state of pin VCC
    delay(50);
    httpserver.send(200, "text/html", "Resetting NRF24...");

  });

  httpserver.on("/send",[](){                                   // Handle inputs from web interface

    if (httpserver.args() > 0 ) {
        for ( uint8_t i = 0; i < httpserver.args(); i++ ) {
          
          if (httpserver.argName(i) == "command") {             // Send command to RFLInk from web interface, check it comes from command input in html form
             String text_command = httpserver.arg(i);           // Get command send
             byte buf[text_command.length() + 1];               // Temp char for conversion
             text_command.getBytes(buf, sizeof(buf));
             rflinkSerialTX.write(MQTT_CHECK_PREFIX);
             rflinkSerialTX.print(F("nrfcmd "));
             rflinkSerialTX.write(buf, sizeof(buf));            // Write command to RFLink serial
             //rflinkSerialTX.print(httpserver.arg(i));
             rflinkSerialTX.print(F("\r\n"));
             DEBUG_PRINTLN();
             DEBUG_PRINTLN(F("=== Web command ==="));
             DEBUG_PRINT(F("message = "));
             DEBUG_WRITE(buf, sizeof(buf));
             DEBUG_PRINTLN(F("\r\n"));
          }
          
       }
    }
    httpserver.sendHeader("Location","/");       	// Add a header to respond with a new location for the browser to go to the home page again
    httpserver.send(303);                       	// Send it back to the browser with an HTTP status 303 (See Other) to redirect
    
  });
  
  httpserver.on("/enable-debug",[](){
    DEBUG_PRINTLN(F("Enabling MQTT debug..."));
    MQTT_DEBUG = 1;
    httpserver.sendHeader("Location","/");      	// Add a header to respond with a new location for the browser to go to the home page again
    httpserver.send(303);                     		// Send it back to the browser with an HTTP status 303 (See Other) to redirect
  });

  httpserver.on("/disable-debug",[](){
    DEBUG_PRINTLN(F("Disabling MQTT debug..."));
    MQTT_DEBUG = 0;
	MQTTClient.publish(MQTT_DEBUG_TOPIC,"{\"DATA\":\" \",\"ID\":\" \",\"NAME\":\" \",\"TOPIC\":\" \",\"JSON\":\" \"}");
    httpserver.sendHeader("Location","/");     		// Add a header to respond with a new location for the browser to go to the home page again
    httpserver.send(303);                       	// Send it back to the browser with an HTTP status 303 (See Other) to redirect
  });
  
  httpserver.on("/data.txt", [](){            		// Used to deliver raw data received (BUFFER) and mqtt data published (MQTT_TOPIC and JSON)           
  if (f_sfwnrf)  {
    if (f_lfwnrf) {
    httpserver.send(200, "text/html", "<td>" + String("Loading NRF in progress ... ") + String(UPLIDX) + " bytes of " + String(UPLBUF_SIZE) + "</td><td>"+ "</td><td>"+ "</td>\r\n");
    f_sfwnrf = false;
    }
    else if (f_ffwnrf) {
    httpserver.send(200, "text/html", "<td>" + String("Loading NRF Failed") + "</td><td>"+ "</td><td>"+ "</td>\r\n");
    f_sfwnrf = false;
    }
    else {
    httpserver.send(200, "text/html", "<td>" + String("Loading NRF Ok") + "</td><td>"+ "</td><td>"+ "</td>\r\n");
    f_sfwnrf = false;
    }
    }
    else {
    httpserver.send(200, "text/html", "<td>" + String(BUFFER) + "</td><td>" + String(MQTT_TOPIC) + "</td><td>" + String(JSON) + "</td>\r\n");
    }
  });

  httpserver.onNotFound([](){
    httpserver.send(404, "text/plain", "404: Not found");      	// Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
  });

};

/**
 * MQTT publish without json
 */
 

/**
 * Time functions
 */
void fwnrfdelay(){
if (t_mqtcon != 0) t_mqtcon--;
if (f_tfwnrf == 0) return;
else {f_tfwnrf--;
//DEBUG_PRINTLN(f_tfwnrf);
        if  (f_tfwnrf == 0) {
        f_lfwnrf = false;
        f_rfwnrf = true;    // no waiting for response
        f_ffwnrf = true;    // load fw error
        f_sfwnrf = true;
        }
      }
}


long uptime_min()
{
 long days=0;
 long hours=0;
 long mins=0;
 long secs=0;
 secs = millis()/1000; 		//convect milliseconds to seconds
 mins=secs/60; 				//convert seconds to minutes
 hours=mins/60; 			//convert minutes to hours
 days=hours/24; 			//convert hours to days
 return mins;
}

String uptime_string_exp()
{
  String result;
  char strUpTime[40];
  int minutes = int(float(millis()) *0.001 / 60);
  int days = minutes / 1440;
  minutes = minutes % 1440;
  int hrs = minutes / 60;
  minutes = minutes % 60;
  sprintf_P(strUpTime, PSTR("%d days %d hours %d minutes"), days, hrs, minutes);
  result = strUpTime;
  return result;
} 

String time_string_exp(long time)
{
  String result;
  char strUpTime[40];
  int minutes = int(float(time) *0.001 / 60);
  int days = minutes / 1440;
  minutes = minutes % 1440;
  //int hrs = minutes / 60;
  int hrs = ( minutes / 60 ) + ( days * 24 );
  minutes = minutes % 60;
  //sprintf_P(strUpTime, PSTR("%d d %d h %d min"), days, hrs, minutes);
  if (hrs > 0) {
	  sprintf_P(strUpTime, PSTR("%d h %d min"), hrs, minutes);
  } else {
	  sprintf_P(strUpTime, PSTR("%d min"), minutes);
  }
  result = strUpTime;
  return result;
} 

/**
 * Miscellaneous
 */



/*********************************************************************************
 * Classic arduino bootstrap
/*********************************************************************************/

void setup() {
   

        // Open serial communications and wait for port to open:
        
        //DEBUG_BEGIN(115200);
        debugSerialTX.begin(38400);        // XXX debug serial : same speed as RFLInk
        DEBUG_PRINTLN();
        DEBUG_PRINTLN(F("Starting..."));
			while (!debugSerialTX) {
					; // wait for serial port to connect. Needed for native USB port only
			}
        DEBUG_PRINTLN(F("Init debug serial done"));

        // Set the data rate for the RFLink serial
        rflinkSerialTX.begin(38400);
        DEBUG_PRINTLN(F("Init rflink serial done"));
        f_lfwnrf = false;   // load fw send disabled
        f_rfwnrf = true;   // no waiting for response
        f_ffwnrf = false;   // load fw error
        f_sfwnrf = false;
        // Setup WIFI
        setup_wifi();

        // Setup MQTT
        MqttConnect();

        // Setup OTA
        SetupOTA();
        ArduinoOTA.begin();
        DEBUG_PRINT(F("Ready for OTA on "));
        DEBUG_PRINT("IP address:\t");
        DEBUG_PRINTLN(WiFi.localIP());

        // Setup HTTP Server
        ConfigHTTPserver();
        httpUpdater.setup(&httpserver);                                 // Firmware webupdate
        //DEBUG_PRINTF("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", MQTT_RFLINK_CLIENT_NAME);
        DEBUG_PRINTLN(F("HTTPUpdateServer ready"));
        httpserver.begin();                                             // Actually start the HTTP server
        DEBUG_PRINTLN(F("HTTP server started")); 

        //Initialize Ticker
        blinker.attach(0.5, fwnrfdelay);
        
        rflinkSerialTX.println();
		delay(100);
        rflinkSerialTX.write(MQTT_CHECK_PREFIX);
        rflinkSerialTX.println(F("nrfcmd ."));                       	// Do an empty cmd on startup
        delay(100);
        // Get values from USER_IDs into matrix
        for (int i =0; i < USER_ID_NUMBER; i++) {
            strcpy(matrix[i].id,USER_IDs[i].id);
            matrix[i].publish_interval = USER_IDs[i].publish_interval;
            strcpy(matrix[i].description,USER_IDs[i].description);
        }

} // setup

/*********************************************************************************
 * Main loop
/*********************************************************************************/

void loop() {
        bool DataReady=false;
        // handle lost of connection : retry after 2s on each loop
        if ((!MQTTClient.connected()) && (t_mqtcon == 0)) {
        //        DEBUG_PRINTLN(F("Not connected, retrying in 2s"));
                t_mqtcon == 4;
                MqttConnect();
                
        } else {
        // handle lost of connection : retry after 1s on each loop
                if ((f_lfwnrf) && (f_rfwnrf)) {
                if (UPLIDX < UPLBUF_SIZE){
                char  i=0;
                char  d=0;
                char  s=16;
                char  al=0;
                char  ah=0;
                al = (UPLIDX & 255);
                ah = ((UPLIDX/256)& 255);
                s = s + ah; s = s + al;
                rflinkSerialTX.write(MQTT_CHECK_PREFIX);
                rflinkSerialTX.print(F(":10"));                
                if (ah < 16) rflinkSerialTX.print(F("0"));
                rflinkSerialTX.print(String(ah, HEX));
                if (al < 16) rflinkSerialTX.print(F("0"));
                rflinkSerialTX.print(String(al, HEX));
                rflinkSerialTX.print(F("00"));
                while(i < 16) {
                d= UPLBUF[UPLIDX]; s=s+d;
                if (d < 16) rflinkSerialTX.print(F("0"));
                rflinkSerialTX.print(String(d, HEX));
                UPLIDX++; i++;
                           }
                s = 256 - s;
                if (s < 16) rflinkSerialTX.print(F("0"));
                rflinkSerialTX.print(String(s, HEX));
                rflinkSerialTX.print(F("\r\n"));
                f_tfwnrf = 10;
                f_rfwnrf = false;
                f_sfwnrf = true;
                      }
                else {                 
                rflinkSerialTX.write(MQTT_CHECK_PREFIX);
                rflinkSerialTX.print(F(":00000001FF"));
                rflinkSerialTX.print(F("\r\n"));
                f_lfwnrf = false;
                f_sfwnrf = true;
                      }
//    httpserver.sendHeader("Location","/");       // Add a header to respond with a new location for the browser to go to the home page again
//    httpserver.send(303);                         // Send it back to the browser with an HTTP status 303 (See Other) to redirect

                
                }
                
                
                // if something arrives from rflink
                if(rflinkSerialRX.available()) {
                        char rc;
                        // bufferize serial message
                        while(rflinkSerialRX.available() && CPT < BUFFER_SIZE) {
                                rc = rflinkSerialRX.read();
                                if (isAscii(rc)) { // ensure char is ascii, this is to stop bad chars being sent https://www.arduino.cc/en/Tutorial/CharacterAnalysis
                                  BUFFER[CPT] = rc;
                                  CPT++;
                                  if (BUFFER[CPT-1] == '\n') {
                                          DataReady = true;
                                          BUFFER[CPT]='\0';
                                          CPT=0;
                                  }
                                }
                        }
                        if (CPT > BUFFER_SIZE ) CPT=0;
                }

                // parse what we just read
                if (DataReady) {
						
						// clean variables
                        strcpy(MQTT_ID,""); strcpy(MQTT_NAME,"");strcpy(MQTT_TOPIC,"");strcpy(JSON,"");strcpy(JSON_DEBUG,"");
						
                        readRfLinkPacket(BUFFER);

            
						if ( (strcmp(MQTT_ID,"") != 0) && (strcmp(MQTT_ID,"0\0") != 0) ) lastReceived = millis();				// Store last received data time if MQTT_ID is valid
                        // construct topic name to publish to
            if  (strcmp(MQTT_ID,"") != 0)  buildMqttTopic(MQTT_NAME, MQTT_ID);
                        // report message for debugging
                        //printToSerial();
                        //if (MQTT_DEBUG) MQTTClient.publish(MQTT_DEBUG_TOPIC,BUFFER);      // XXX MQTT debug mode, raw data only
						if (MQTT_DEBUG) {													// XXX MQTT debug mode with json, full data
							strcpy(BUFFER_DEBUG,"{");
							strcat(BUFFER_DEBUG,"\"DATA\":\"");strncat(BUFFER_DEBUG,BUFFER,strlen(BUFFER)-2);strcat(BUFFER_DEBUG,"\"");
							strcat(BUFFER_DEBUG,",\"ID\":\"");strcat(BUFFER_DEBUG,MQTT_ID);strcat(BUFFER_DEBUG,"\"");
							strcat(BUFFER_DEBUG,",\"NAME\":\"");strcat(BUFFER_DEBUG,MQTT_NAME);strcat(BUFFER_DEBUG,"\"");
							strcat(BUFFER_DEBUG,",\"TOPIC\":\"");strcat(BUFFER_DEBUG,MQTT_TOPIC);strcat(BUFFER_DEBUG,"\"");
							strcpy(JSON_DEBUG,JSON);
							for (char* p = JSON_DEBUG; p = strchr(p, '\"'); ++p) {*p = '\'';}	// Remove quotes
							strcat(BUFFER_DEBUG,",\"JSON\":\"");strcat(BUFFER_DEBUG,JSON_DEBUG);strcat(BUFFER_DEBUG,"\"");
							strcat(BUFFER_DEBUG,"}");
							MQTTClient.publish(MQTT_DEBUG_TOPIC,BUFFER_DEBUG);
						}
                        
                        // XXX publish to MQTT server only if ID is authorized
                        
                        if (USER_ID_NUMBER > 0) {
                          
                          int i;
                          for (i = 0; i < (USER_ID_NUMBER); i++){
                            
                            DEBUG_PRINT(F("ID = "));DEBUG_PRINT(matrix[i].id);
                            DEBUG_PRINT(F(" ; publish_interval = "));DEBUG_PRINT(matrix[i].publish_interval);
                            DEBUG_PRINT(F(" ; decription = "));DEBUG_PRINT(matrix[i].description);
                            DEBUG_PRINT(F(" ; json = "));DEBUG_PRINT(matrix[i].json);
                            DEBUG_PRINT(F(" ; last_published = "));DEBUG_PRINT(matrix[i].last_published);
                            DEBUG_PRINT(F(" ; last_received = "));DEBUG_PRINTLN(matrix[i].last_received);
                            
                            if (strcmp(MQTT_ID,matrix[i].id) == 0) {                                      	// check ID is authorized
                              DEBUG_PRINT(F("Authorized ID "));DEBUG_PRINT(MQTT_ID);
                              matrix[i].last_received = millis();                                         	// memorize received time
                              if (strcmp(matrix[i].json,JSON) != 0) {                                     	// check if json value has changed
                                DEBUG_PRINT(F(" => data changed => published on "));DEBUG_PRINTLN(MQTT_TOPIC);
                                MQTTClient.publish(MQTT_TOPIC,JSON);                           			  	// if changed, publish on MQTT server  
                                strcpy(matrix[i].json,JSON);                                              	// memorize new json value
                                matrix[i].last_published = millis();                                      	// memorize published time                                
                              } else {                                                                    	// no value change
                                now = millis();
                                if ( (now - matrix[i].last_published) > (matrix[i].publish_interval) ) {   	// check if it exceeded time for last publish
                                  DEBUG_PRINT(F(" => no data change but max time interval exceeded => published on "));DEBUG_PRINTLN(MQTT_TOPIC);
                                  if  (strcmp(MQTT_ID,"") != 0) MQTTClient.publish(MQTT_TOPIC,JSON);                             			// publish on MQTT server
                                  matrix[i].last_published = millis();                               		// memorize published time
                                } else {
                                  DEBUG_PRINTLN(" => no data change => not published");
                                }
                              }   // no data changed
                            }     // authorized id
                            
                          }       // for
                          
                        } else {                                                	// case all IDs are authorized
                          if  (strcmp(MQTT_ID,"") != 0) MQTTClient.publish(MQTT_TOPIC,JSON);                   	// publish on MQTT server
                        }
                        DEBUG_PRINTLN();
                        //pubFlatMqtt(MQTT_TOPIC,JSON);								// expands JSON and publish on several topics
                } // end of DataReady

				now = millis();
				
                // Handle uptime

                if ( (now - lastUptime) > (uptimeInterval) ) {    					// if uptime interval is exceeded
					char mqtt_publish_payload[50];
					lastUptime = now;
					String mqtt_publish_string = String(uptime_min());
					mqtt_publish_string.toCharArray(mqtt_publish_payload, mqtt_publish_string.length() + 1);
					MQTTClient.publish(MQTT_UPTIME_TOPIC,mqtt_publish_payload);
					DEBUG_PRINT("Uptime : ");DEBUG_PRINTLN(uptime_string_exp());
                }
				  
				// Handle Mega reset if no data is received
				if (resetMegaInterval > 0) {										// only if enabled
					if ( (now - lastReceived) > (resetMegaInterval) ) {    			// if time interval exceeded, reset Mega
						if (MQTT_DEBUG) {
							char mqtt_publish_payload[50];
							String mqtt_publish_string;
							mqtt_publish_string = "{\"DATA\":\"NO DATA FOR " + time_string_exp(now - lastReceived) + " : RESET NRF\"}";
							mqtt_publish_string.toCharArray(mqtt_publish_payload, mqtt_publish_string.length() + 1);
							MQTTClient.publish(MQTT_DEBUG_TOPIC,mqtt_publish_payload);
						}
						DEBUG_PRINT(F("No data received for "));DEBUG_PRINT(time_string_exp(now - lastReceived));DEBUG_PRINTLN(F(": Resetting Mega"));
						pinMode(NRF_RESET_PIN, OUTPUT);
						digitalWrite(NRF_RESET_PIN,false);                         // Change the state of pin to ground
						delay(1000); 
						digitalWrite(NRF_RESET_PIN,true);                          // Change the state of pin to VCC
						delay(50);
						lastReceived = millis();									// Fake the last received time to avoid permanent reset
					  }
				}
                // Handle MQTT callback
                MQTTClient.loop();
                
        } // else (=connected on MQTT server)
        
        ArduinoOTA.handle();                                            // Listen for OTA update
        httpserver.handleClient();                                      // Listen for HTTP requests from clients
        
} // loop

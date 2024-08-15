#ifndef CONFIG_H
#define CONFIG_H

// Wifi: SSID and password
const PROGMEM char* WIFI_SSID = "[Redacted]";
const PROGMEM char* WIFI_PASSWORD = "[Redacted]";

// If 0.0.0.0 use dhcp
const IPAddress local_IP(0,0,0,0);

// If neded set your Static IP address
// const IPAddress local_IP(192, 168, 0, 54);
// Set your Gateway IP address
// const IPAddress gateway(192, 168, 0, 1);
const IPAddress subnet(255, 255, 255, 0);
// Optional DNS
// IPAddress primaryDNS(8, 8, 8, 8);   //optional
// IPAddress secondaryDNS(8, 8, 4, 4); //optional

// MQTT: ID, server IP, port, username and password
const PROGMEM char* MQTT_CLIENT_ID = "somewhere_relay01";
const PROGMEM char* MQTT_SERVER_IP = "192.168.0.5";
const PROGMEM uint16_t MQTT_SERVER_PORT = 1883;
const PROGMEM char* MQTT_USER = "[Redacted]";
const PROGMEM char* MQTT_PASSWORD = "[Redacted]";

// MQTT: topics
const PROGMEM char* MQTT_WILL_TOPIC = "home/somewhere/relay01/will";
const PROGMEM char* MQTT_SENSOR_TOPIC = "home/somewhere/relay01/sensor";
const PROGMEM char* MQTT_LOG_TOPIC = "home/somewhere/relay01/log";
const PROGMEM char* MQTT_RELAY_GET_TOPIC = "home/somewhere/relay01/get";

const PROGMEM char* MQTT_REALY_POWER_SET_TOPIC = "home/somewhere/relay01/power/set";
const PROGMEM char* MQTT_RELAY_MODE_SET_TOPIC = "home/somewhere/relay01/mode/set";
const PROGMEM char* MQTT_RELAY_TEMP_SET_TOPIC = "home/somewhere/relay01/temp/set";

const PROGMEM char* COMM_ON = "on";
const PROGMEM char* COMM_OFF = "off";
const PROGMEM char* COMM_AUTO = "auto";


// sleeping time
const uint16_t SLEEPING_TIME_IN_MSECONDS = 500; 

// const float temp_slope = 1.0f;
// const float temp_shift = 0.0f;
// const float humid_slope = 1.0f;
// const float humid_shift = 0.0f;

const float temp_slope = 1.005f;
const float temp_shift = -1.746f;
const float humid_slope = 1.775f;
const float humid_shift = -38.07f;

const std::size_t n_reads = 3;

#define INIT_TARGET_TEMP 21.0

static const int mqtt_max_attempt = 10;

#ifdef  ARDUINO_ESP8266_GENERIC
#define D2 4
#define D3 0
#define D4 2
#define D9 3
#endif

#define DHTPIN D4
#define DHTTYPE DHT11

// GPIO4 (D2) and GPIO5 (D1) are the most safe to use GPIOs if you want to operate relays 
// (https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/)
//#define RELAYPIN D2
#define RELAYPIN D1

#endif

//ESP32
//#define PIN_CLK  22
//#define PIN_DATA 21

//ESP8266
#define PIN_CLK  5
#define PIN_DATA 4


/**
 * If you're using a cheap D1 clone, these might be the correct pins for you
 * 
 */
//#define PIN_CLK  4 //D2 on PCB
//#define PIN_DATA 5 //D1 on PCB

const float EXP_SMOOTH_ALPHA = 0.125;

const unsigned long PUBLISH_INTERVAL_FAST_MS = (10 * 1000);
const unsigned long PUBLISH_INTERVAL_SLOW_MS = (3 * 60 * 1000);

const char* MQTT_HOST = "192.168.178.47";
const char* MQTT_USERNAME = NULL;
const char* MQTT_PASSWORD = NULL;

const uint8_t MQTT_MAX_CONNECT_RETRY = 10;

//If you don't want to use home-assistant autodiscovery comment this out
//#define USE_HA_AUTODISCOVERY


#ifdef USE_HA_AUTODISCOVERY
  #define HA_DISCOVERY_PREFIX "homeassistant"
  const char* MQTT_LAST_WILL_PAYLOAD_CONNECTED = "online";
  const char* MQTT_LAST_WILL_PAYLOAD_DISCONNECTED = "offline";
#else
  //If you're not using HA Autodiscovery, you can specify your topics here

  const char* MQTT_TOPIC_CO2_MEASUREMENT = "sensor/co2monitor/co2";
  const char* MQTT_TOPIC_TEMPERATURE_MEASUREMENT = "sensor/co2monitor/temperature";
  const char* MQTT_TOPIC_HUMIDITY_MEASUREMENT = "sensor/co2monitor/humidity";
  const char* MQTT_TOPIC_LAST_WILL = "state/sensor/co2monitor/status";
  const char* MQTT_LAST_WILL_PAYLOAD_CONNECTED = "connected";
  const char* MQTT_LAST_WILL_PAYLOAD_DISCONNECTED = "disconnected";
#endif


//These are optional 
//const char* HOSTNAME = "SSID"; //Put Wifi Name here
//const char* CONF_WIFI_PASSWORD = "Password"; //Put your Password here

#define OTA_PASSWORD "Ubuntu"

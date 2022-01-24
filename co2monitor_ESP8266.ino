//ESP32
//#include <Arduino.h>
//#include <WiFi.h>
//#include <ArduinoOTA.h>
//#include <DNSServer.h>
//#include <WebServer.h>

//ESP8266
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

#include <PubSubClient.h>
#include <WiFiManager.h>

#include "settings.h"

#define IDX_CMD 0
#define IDX_MSB 1
#define IDX_LSB 2
#define IDX_CHECKSUM 3
#define IDX_END 4

#define CMD_HUMIDITY 0x41
#define CMD_TEMPERATURE 0x42
#define CMD_CO2_MEASUREMENT 0x50

#ifdef USE_HA_AUTODISCOVERY
#define FIRMWARE_PREFIX "esp8266-co2monitor"
char MQTT_TOPIC_LAST_WILL[128];
char MQTT_TOPIC_CO2_MEASUREMENT[128];
char MQTT_TOPIC_TEMPERATURE_MEASUREMENT[128];
char MQTT_TOPIC_HUMIDITY_MEASUREMENT[128];
#endif

WiFiClient wifiClient;
PubSubClient mqttClient;

uint8_t bitIndex = 0;
uint8_t byteIndex = 0;
uint8_t clkValue = LOW;
uint8_t lastClkValue = LOW;

uint8_t tmp = 0;
unsigned long currentMillis = 0;
unsigned long lastMillis = 0;
unsigned long lastUpdateMs = 0;
uint8_t mqttRetryCounter = 0;

uint16_t co2Measurement = 0;
float smoothCo2Measurement = 0.0;

float temperature = 0;
float humidity = 0;

byte bits[8];
byte bytes[5] = {0};

char sprintfHelper[16] = {0};

char hostname[16];

void setup() {
  Serial.begin(115200);
  Serial.println("\n");
  Serial.println("Hello from esp8266-co2monitor");
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level

  // Power up wait
  delay(2000);

  WiFiManager wifiManager;
  //ESP32
  //uint32_t chipid = ESP.getEfuseMac();
  //ESP8266
  int32_t chipid = ESP.getChipId();

  Serial.print("MQTT_MAX_PACKET_SIZE: ");
  Serial.println(MQTT_MAX_PACKET_SIZE);


#ifdef HOSTNAME
  hostname = HOSTNAME;
#else
  snprintf(hostname, 24, "CO2MONITOR-%X", chipid);
  //kurzere version
  //hostname = "CO2MON";
#endif

#ifdef USE_HA_AUTODISCOVERY
  snprintf(MQTT_TOPIC_LAST_WILL, 127, "%s/%s/presence", FIRMWARE_PREFIX, hostname);
  snprintf(MQTT_TOPIC_CO2_MEASUREMENT, 127, "%s/%s/%s_%s/state", FIRMWARE_PREFIX, hostname, hostname, "co2");
  snprintf(MQTT_TOPIC_TEMPERATURE_MEASUREMENT, 127, "%s/%s/%s_%s/state", FIRMWARE_PREFIX, hostname, hostname, "temp");
  snprintf(MQTT_TOPIC_HUMIDITY_MEASUREMENT, 127, "%s/%s/%s_%s/state", FIRMWARE_PREFIX, hostname, hostname, "hum");
#endif

  wifiManager.setConfigPortalTimeout(120);
#ifdef CONF_WIFI_PASSWORD
  wifiManager.autoConnect(hostname, CONF_WIFI_PASSWORD);
#else
  wifiManager.autoConnect(hostname);
#endif
  //ESP32
  //WiFi.getHostname();
  //ESP8266
  WiFi.hostname(hostname);

  mqttClient.setClient(wifiClient);
  mqttClient.setServer(MQTT_HOST, 1883);

  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.begin();

  pinMode(PIN_CLK, INPUT);
  pinMode(PIN_DATA, INPUT);

  attachInterrupt(PIN_CLK, onClock, RISING);

  Serial.print("Hostname: ");
  Serial.println(hostname);

  Serial.println("-- Current GPIO Configuration --");
  Serial.print("PIN_CLK: ");
  Serial.println(PIN_CLK);
  Serial.print("PIN_DATA: ");
  Serial.println(PIN_DATA);

  mqttConnect();
}

ICACHE_RAM_ATTR void onClock() {

  lastMillis = millis();
  bits[bitIndex++] = (digitalRead(PIN_DATA) == HIGH) ? 1 : 0;

  // Transform bits to byte
  if (bitIndex >= 8) {
    tmp = 0;
    for (uint8_t i = 0; i < 8; i++) {
      tmp |= (bits[i] << (7 - i));
    }

    bytes[byteIndex++] = tmp;
    bitIndex = 0;
  }

  if (byteIndex >= 5) {
    byteIndex = 0;
    decodeDataPackage(bytes);
  }
}

void mqttConnect() {
  while (!mqttClient.connected()) {

    bool mqttConnected = false;
    if (MQTT_USERNAME && MQTT_PASSWORD) {
      mqttConnected = mqttClient.connect(hostname, MQTT_USERNAME, MQTT_PASSWORD, MQTT_TOPIC_LAST_WILL, 1, true, MQTT_LAST_WILL_PAYLOAD_DISCONNECTED);
    } else {
      mqttConnected = mqttClient.connect(hostname, MQTT_TOPIC_LAST_WILL, 1, true, MQTT_LAST_WILL_PAYLOAD_DISCONNECTED);
    }

    if (mqttConnected) {
      Serial.println("Connected to MQTT Broker");
      mqttClient.publish(MQTT_TOPIC_LAST_WILL, MQTT_LAST_WILL_PAYLOAD_CONNECTED, true);
      mqttRetryCounter = 0;

    } else {
      Serial.println("Failed to connect to MQTT Broker");

      if (mqttRetryCounter++ > MQTT_MAX_CONNECT_RETRY) {
        //Serial.println("Restarting uC");
        //ESP.restart();
        Serial.println("Going to DeepSleepfor 15 min");
        //DeepSleep in microsekunden (alle 15 min ueberpruefen ob Netzwerk wieder erreichbar)
        ESP.deepSleep(900 * 1000000);
      }

      delay(2000);
    }
  }
}


void loop() {
  currentMillis = millis();

  // Over 50ms no bits? Reset!
  if (currentMillis - lastMillis > 50) {
    bitIndex = 0;
    byteIndex = 0;
  }

  long updateInterval = PUBLISH_INTERVAL_SLOW_MS;

  // If the change is above a specific threshold, we update faster!
  float percentChange = abs(((float) co2Measurement / smoothCo2Measurement) - 1.0);
  if (percentChange > 0.05) {
    updateInterval = PUBLISH_INTERVAL_FAST_MS;
  }

  if (currentMillis - lastUpdateMs > updateInterval) {
    lastUpdateMs = millis();
    
    if (co2Measurement > 0) {
      sprintf(sprintfHelper, "%d", co2Measurement);
      mqttClient.publish(MQTT_TOPIC_CO2_MEASUREMENT, sprintfHelper, true);
    }

    if (temperature > 0) {
      dtostrf(temperature, 4, 2, sprintfHelper);
      mqttClient.publish(MQTT_TOPIC_TEMPERATURE_MEASUREMENT, sprintfHelper, true);
    }

    if (humidity > 0) {
      dtostrf(humidity, 4, 2, sprintfHelper);
      mqttClient.publish(MQTT_TOPIC_HUMIDITY_MEASUREMENT, sprintfHelper, true);
    }
    

    if ((co2Measurement > 0) && (temperature > 0) && (humidity > 0)) {
        Serial.println("send sensor data via mqtt:");
        mqttConnect();
        mqttClient.loop();
        delay(500);
        
        Serial.print(MQTT_TOPIC_CO2_MEASUREMENT);
        Serial.print(": ");
        Serial.println(co2Measurement);
        Serial.print(MQTT_TOPIC_TEMPERATURE_MEASUREMENT);
        Serial.print(": ");
        Serial.println(temperature);
        Serial.print(MQTT_TOPIC_HUMIDITY_MEASUREMENT);
        Serial.print(": ");
        Serial.println(humidity);
        digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
        Serial.println("Going to DeepSleep for 3 min");
        //DeepSleep in microsekunden (alle 3 min wird Sensor Wert gelesen und ueber mqtt verschickt)
        ESP.deepSleep(180 * 1000000);
      } else {
        Serial.println("no sensor data available");
      }
  }

  mqttConnect();
  mqttClient.loop();

  ArduinoOTA.handle();


}

bool decodeDataPackage(byte data[5]) {

  if (data[IDX_END] != 0x0D) {
    Serial.println("wrong end byte");
    return false;
  }

  uint8_t checksum = data[IDX_CMD] + data[IDX_MSB] + data[IDX_LSB];
  if (data[IDX_CHECKSUM] != checksum) {
    Serial.println("wrong checksum");
    return false;
  }

  switch (data[IDX_CMD]) {
    case CMD_CO2_MEASUREMENT:
      co2Measurement = (data[IDX_MSB] << 8) | data[IDX_LSB];

      // Exponential smoothing
      smoothCo2Measurement = EXP_SMOOTH_ALPHA * (float) co2Measurement + (1.0 - EXP_SMOOTH_ALPHA) * smoothCo2Measurement;

      Serial.print("CO2: ");
      Serial.println(co2Measurement);
      break;
    case CMD_TEMPERATURE:
      temperature = ((data[IDX_MSB] << 8) | data[IDX_LSB]) / 16.0 - 273.15;
      Serial.print("Temp: ");
      Serial.println(temperature);
      break;
    case CMD_HUMIDITY:
      humidity = ((data[IDX_MSB] << 8) | data[IDX_LSB]) / 100.0;
      Serial.print("Hum: ");
      Serial.println(humidity);
      break;
  }
}

# ESP8266-co2monitor

## description

Interface an TFA Dostmann CO2 Monitor (Model: AirCO2ntrol Coach) with ESP8266 (in my example ESP-12) to connect it to the "Internet of Things", fully integrated into the case. It publishes the CO2 measurement and temperature to a configured MQTT topic.

Not working:
- [Home Assistant Auto Discovery](https://www.home-assistant.io/docs/mqtt/discovery/).

Thanks to "schinken" for the main part of the work:
https://github.com/schinken/esp8266-co2monitor


## compiling

* configure settings.h
* Open up your arduino IDE
* Upload the compiled result to your ESP6288

## wiring

Add pins to J-SPI: G, DATA, CLK, +3V (= Ground / DATA / CLOCK / 3.3 V):
<br>
<a href="https://github.com/timo619/co2monitor/blob/master/doc/images/pins.jpg?raw=true">
    <img alt="Pins" src="https://github.com/timo619/co2monitor/blob/master/doc/images/pins.jpg?raw=true">
</a>

Prepare wiring of our ESP8266 as seen here (with RST to GPIO16, if you are using the ESP.deepSleep function):
<br>
<a href="https://github.com/timo619/co2monitor/blob/master/doc/images/wiring.jpg?raw=true">
    <img alt="Wemos Wiring" src="https://github.com/timo619/co2monitor/blob/master/doc/images/wiring.jpg?raw=true">
</a>

Insert an isolate your ESP8266 to your co2 monitor:
<br>
<a href="https://github.com/timo619/co2monitor/blob/master/doc/images/esp12_inside.jpg?raw=true">
    <img alt="Final Wiring" src="https://github.com/timo619/co2monitor/blob/master/doc/images/esp12_inside.jpg?raw=true">
</a>

## using
Since we're using [WiFiManager](https://github.com/tzapu/WiFiManager), after flashing, the EPS8266 will open up a WiFi Hotspot to configure Wireless credentials. Just connect to it using a wifi-capable device and open 
http://192.168.4.1 (no https)

The ESP8266 will also default to AP mode if the configured wifi is unavailable.
You might want to set a password for the AP mode in your settings.h

## dependencies

* Arduino/ESP8266

## notes

Also works on ESP32 when some out-commented functions are enabled

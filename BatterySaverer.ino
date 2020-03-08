#include <DallasTemperature.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define SLEEP_TIME 120000000

#define WLAN_SSID "YOURSSID"
#define WLAN_PASSWD "YourWiFiPassword"

/************************* Adafruit.io Setup *********************************/

#define MQTT_SERVER      "io.adafruit.com"
#define MQTT_SERVERPORT  1883 
#define MQTT_USERNAME    "yourUserName"
#define MQTT_PASSWD         "yourPassword"
const char MQTT_CLIENTID[] PROGMEM = __TIME__ MQTT_USERNAME;

// Dallas Temperature
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);				// Setup a oneWire instance to communicate with any OneWire device
DallasTemperature tempSensors(&oneWire);	// Pass oneWire reference to DallasTemperature library


int loops = 0;
void setup()
{
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    delay(1);
    Serial.begin(115200);
    while (!Serial)
        ;
    Serial.println("Booted, and have disabled WiFi radio");
}
void loop()
{
    loops++;
    Serial.println("Entering loop()");
    ConnectToWiFi();
    MQTT_uploadToServer();
    DisconnectWiFi();
    Serial.println("Going to sleep");
    ESP.deepSleep(SLEEP_TIME, WAKE_RF_DISABLED);
    Serial.println("Woke up");
}

void DisconnectWiFi()
{
    WiFi.disconnect(true);
    delay(1);
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    delay(5);
}
void ConnectToWiFi()
{
    Serial.println("Enabling WiFi");
    WiFi.forceSleepWake();
    delay(1);
    WiFi.persistent(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WLAN_SSID, WLAN_PASSWD);
    int retries = 0;
    int wifiStatus = WiFi.status();
    while (wifiStatus != WL_CONNECTED)
    {
        retries++;
        if (retries == 300)
        {
            Serial.println("No connection after 300 steps, powercycling the WiFi radio. I have seen this work when the connection is unstable");
            WiFi.disconnect();
            delay(10);
            WiFi.forceSleepBegin();
            delay(10);
            WiFi.forceSleepWake();
            delay(10);
            WiFi.begin(WLAN_SSID, WLAN_PASSWD);
        }
        if (retries == 600)
        {
            WiFi.disconnect(true);
            delay(1);
            WiFi.mode(WIFI_OFF);
            WiFi.forceSleepBegin();
            delay(10);
            if (loops == 3)
            {
                Serial.println("That was 3 loops, still no connection so let's go to deep sleep for 2 minutes");
                Serial.flush();
                ESP.deepSleep(SLEEP_TIME, WAKE_RF_DISABLED);
                loops = 0;
            }
            else
            {
                Serial.println("No connection after 600 steps. WiFi connection failed, disabled WiFi and waiting for a minute");
            }
            delay(60000);
            return;
        }
        delay(50);
        wifiStatus = WiFi.status();
    }
    Serial.print("Connected to ");
    Serial.println(WLAN_SSID);
    Serial.print("Assigned IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("WiFi connection successful, waiting for 10 seconds");
    delay(10000);
}
void MQTT_uploadToServer()
{
    WiFiClient client;
    Serial.println("Sending to MQTT.");
    tempSensors.requestTemperatures();
    float tempF1 = (tempSensors.getTempCByIndex(1) * 9.0) / 5.0 + 32.0;
    float tempF2 = (tempSensors.getTempCByIndex(0) * 9.0) / 5.0 + 32.0;

    Serial.print("tempF1 -> ");
    Serial.println(tempF1);
    Serial.print("tempF2 -> ");
    Serial.println(tempF2);

    int8_t ret;
    Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_SERVERPORT, MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWD);
    Serial.println("Connecting to MQTT... ");
    uint8_t retries = 3;
    while ((ret = mqtt.connect()) != 0)
    { // connect will return 0 for connected
        Serial.println(mqtt.connectErrorString(ret));
        Serial.println("Retrying MQTT connection in 5 seconds...");
        mqtt.disconnect();
        delay(5000); // wait 5 seconds
        retries--;
        if (retries == 0)
        {
            // basically just hang around and wait for the watchdog to come and eat us.
            while (1)
                ;
        }
    }
    Serial.println("MQTT Connected!");
    Serial.println("Sending MQTT message");
    mqtt.publish("yourAdafruitIOUser/f/yourFeed1", String(tempF1).c_str());
    mqtt.publish("yourAdafruitIOUser/f/yourFeed2", String(tempF2).c_str());
    delay(50);
    mqtt.disconnect();
    Serial.println("Disconnected from MQTT");
}
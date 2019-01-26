/*
   MQTT RGB Light for Home-Assistant - NodeMCU (ESP8266)
   https://home-assistant.io/components/light.mqtt/

   Configuration (HA) : 
    light:
    - platform: mqtt
      name: 'Office RGB light'
      state_topic: 'liquor-lights/top/status'
      command_topic: 'liquor-lights/top/switch'
      brightness_state_topic: 'liquor-lights/top/brightness/status'
      brightness_command_topic: 'liquor-lights/top/brightness/set'
      rgb_state_topic: 'liquor-lights/top/rgb/status'
      rgb_command_topic: 'liquor-lights/top/rgb/set'
      brightness_scale: 100
      optimistic: false
    - platform: mqtt
      name: 'Office RGB light'
      state_topic: 'liquor-lights/btm/status'
      command_topic: 'liquor-lights/btm/switch'
      brightness_state_topic: 'liquor-lights/btm/brightness/status'
      brightness_command_topic: 'liquor-lights/btm/brightness/set'
      rgb_state_topic: 'liquor-lights/btm/rgb/status'
      rgb_command_topic: 'liquor-lights/btm/rgb/set'
      brightness_scale: 100
      optimistic: false
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NeoPixelBus.h>
#include "light.h"
#include "secrets.h"

#define LEDCOUNT   69       // Number of LEDs used for serial
#define MQTT_VERSION MQTT_VERSION_3_1_1

const char* WIFI_SSID = MY_WIFI_SSID;
const char* WIFI_PASSWORD = MY_WIFI_PASS;

const PROGMEM char* MQTT_CLIENT_ID = "LIQUOR-LIGHTS";
const PROGMEM char* MQTT_SERVER_IP = MY_MQTT_SERVER_IP;
const PROGMEM uint16_t MQTT_SERVER_PORT = 1883;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
PubSubClient* mqtt_ptr = &mqtt;
NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart800KbpsMethod> strip(LEDCOUNT);

Light* top_light;
Light* btm_light;

// function called when a MQTT message arrived
void mqtt_message_received(char* topic, byte* payload_bytes, unsigned int payload_length) {
    String payload;
    for (uint8_t i = 0; i < payload_length; i++) {
        payload.concat((char)payload_bytes[i]);
    }
    Serial.print("Message Received: ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(payload);

  top_light->processMessage(String(topic), payload);
  btm_light->processMessage(String(topic), payload);
}


void setTop(RgbColor* pixels) {
    for (int i = 0; i < 35; i++) {
        strip.SetPixelColor(i + 34, pixels[i]);
    }
}
void setBtm(RgbColor* pixels) {
    for (int i = 0; i < 34; i++) {
        strip.SetPixelColor(i, pixels[i]);
    }
}

void setup() {
  Serial.begin(115200);
  strip.Begin();

  // init the WiFi connection
  Serial.println();
  Serial.println();
  Serial.print("INFO: Connecting to ");
  WiFi.mode(WIFI_STA);
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("INFO: WiFi connected");
  Serial.print("INFO: IP address: ");
  Serial.println(WiFi.localIP());

  // init the MQTT connection
  mqtt.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
  mqtt.setCallback(mqtt_message_received);

  top_light = new Light("liquor-lights/top", 35, &mqtt);
  btm_light = new Light("liquor-lights/btm", 34, &mqtt);
}

void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }
  setBtm(btm_light->render());
  setTop(top_light->render());
  strip.Show();
  mqtt.loop();
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.println("INFO: Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(MQTT_CLIENT_ID)) {
      Serial.println("INFO: connected");
      
      // Once connected, publish an announcement...
      // publish the initial values
      top_light->publishState();
      top_light->publishBrightness();
      top_light->publishRGBColor();

      btm_light->publishState();
      btm_light->publishBrightness();
      btm_light->publishRGBColor();

      // ... and resubscribe
      top_light->subscribe();
      btm_light->subscribe();

    } else {
      Serial.print("ERROR: failed, rc=");
      Serial.print(mqtt.state());
      Serial.println("DEBUG: try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

#ifndef Light_h
#define Light_h

#include <NeoPixelBus.h>
#include <PubSubClient.h>
#include "Arduino.h"

using namespace std;

const uint8_t MSG_BUFFER_SIZE = 20;

class Light
{
    public:
        Light(const char* topic_prefix, int number_of_pixels, PubSubClient* mqtt);
        void publishState();
        void publishBrightness();
        void publishRGBColor();
        void publishEffect();
        void processMessage(String topic, String payload);
        void subscribe();
        RgbColor currentColor();
        RgbColor* render();

        bool state;                     // Current mode: ON/OFF
        uint8_t target_brightness;       // brightness
        uint8_t prev_brightness;         // previous brightness
        RgbColor target_color;           // target (and after the fade, the current) color set by MQTT
        RgbColor prev_color;             // the previous color set by MQTT
        bool is_fading;                  // whether or not we are fading between MQTT colors
        float fade_progress;             // how far we have faded
        float fade_step;                 // how much the fade progress should change each iteration
        String active_effect;

    private:
        void fadeTo(RgbColor color, uint8_t brightness);
        void fadeTo(RgbColor color, uint8_t brightness, float step);
        RgbColor calculateFade(RgbColor fromColor, RgbColor toColor, uint8_t fromBrightness, uint8_t toBrightness, float progress);
        RgbColor applyBrightness(RgbColor color, uint8_t brightness);
        const char* prefixIt(const char* prefix, const char* str);

        int num_pixels;
        RgbColor* pixels;
        PubSubClient* mqtt;
        char msg_buffer[MSG_BUFFER_SIZE];

        const char* status_topic;
        const char* command_topic;
        const char* brightness_status_topic;
        const char* brightness_command_topic;
        const char* rgb_status_topic;
        const char* rgb_command_topic;
        const char* effect_status_topic;
        const char* effect_command_topic;
};

#endif
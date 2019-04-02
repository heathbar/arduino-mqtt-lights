#include <NeoPixelBus.h>
#include "Arduino.h"
#include "light.h"
#include "rainbow.h";

using namespace std;

const PROGMEM char* LIGHT_ON = "ON";
const PROGMEM char* LIGHT_OFF = "OFF";

Light::Light(const char* topic_prefix, int number_of_pixels, PubSubClient* mqtt_client)
{
    status_topic = prefixIt(topic_prefix, "/status");
    command_topic = prefixIt(topic_prefix, "/switch");
    brightness_status_topic = prefixIt(topic_prefix, "/brightness/status");
    brightness_command_topic = prefixIt(topic_prefix, "/brightness/set");
    rgb_status_topic = prefixIt(topic_prefix, "/rgb/status");
    rgb_command_topic = prefixIt(topic_prefix, "/rgb/set");
    effect_status_topic = prefixIt(topic_prefix, "/effect/status");
    effect_command_topic = prefixIt(topic_prefix, "/effect/set");

    num_pixels = number_of_pixels;
    pixels = (RgbColor*) malloc(sizeof(RgbColor) * num_pixels);
    mqtt = mqtt_client;

    state = true;
    target_color = RgbColor(0, 0, 48);
    target_brightness = 255;
    active_effect = String("none");
}

void Light::subscribe()
{
    mqtt->subscribe(command_topic);
    mqtt->subscribe(brightness_command_topic);
    mqtt->subscribe(rgb_command_topic);
    mqtt->subscribe(effect_command_topic);
}

void Light::publishState()
{
    if (state) {
        mqtt->publish(status_topic, LIGHT_ON, true);
    } else {
        mqtt->publish(status_topic, LIGHT_OFF, true);
    }
}

void Light::publishBrightness()
{
    snprintf(msg_buffer, MSG_BUFFER_SIZE, "%d", target_brightness);
    mqtt->publish(brightness_status_topic, msg_buffer, true);
}

void Light::publishRGBColor()
{
    snprintf(msg_buffer, MSG_BUFFER_SIZE, "%d,%d,%d", target_color.R, target_color.G, target_color.B);
    mqtt->publish(rgb_status_topic, msg_buffer, true);
}

void Light::publishEffect()
{
    mqtt->publish(effect_status_topic, active_effect.c_str(), true);
}

void Light::processMessage(String topic, String payload)
{
    if (topic.equals(command_topic)) {
        // test if the payload is equal to "ON" or "OFF"
        if (payload.equals(String(LIGHT_ON))) {
            if (state != true) {
                state = true;
                fadeTo(prev_color, prev_brightness, 0.001);
                publishState();
                publishBrightness();
                publishRGBColor();
            }
        } else if (payload.equals(String(LIGHT_OFF))) {
            if (state != false) {
                state = false;
                fadeTo(RgbColor(0, 0, 0), 0, 0.001);
                publishState();
            }
        }
    } else if (topic.equals(brightness_command_topic)) {
        uint8_t brightness = payload.toInt();
        if (brightness < 0 || brightness > 255) {
            return;
        }

        if (state) {
            fadeTo(target_color, brightness);
            publishBrightness();
        } else {
            prev_brightness = brightness;
        }

    } else if (topic.equals(rgb_command_topic)) {
        // get the position of the first and second commas
        uint8_t firstIndex = payload.indexOf(',');
        uint8_t lastIndex = payload.lastIndexOf(',');
        
        uint8_t rgb_red = payload.substring(0, firstIndex).toInt();
        if (rgb_red < 0 || rgb_red > 255) {
            return;
        }
        
        uint8_t rgb_green = payload.substring(firstIndex + 1, lastIndex).toInt();
        if (rgb_green < 0 || rgb_green > 255) {
            return;
        }
        
        uint8_t rgb_blue = payload.substring(lastIndex + 1).toInt();
        if (rgb_blue < 0 || rgb_blue > 255) {
            return;
        }

        if (state) {
            fadeTo(RgbColor(rgb_red, rgb_green, rgb_blue), target_brightness);
            publishRGBColor();
            active_effect = "none";
            publishEffect();
        } else {
            prev_color = RgbColor(rgb_red, rgb_green, rgb_blue);
        }
    } else if (topic.equals(effect_command_topic)) {
        active_effect = payload;
        publishEffect();
    }
}

void Light::fadeTo(RgbColor color, uint8_t brightness) {
    fadeTo(color, brightness, 0.003);
}

void Light::fadeTo(RgbColor color, uint8_t brightness, float step) {
    prev_color = target_color;
    target_color = color;
    prev_brightness = target_brightness;
    target_brightness = brightness;
    is_fading = true;
    fade_progress = 0;
    fade_step = step;
}

RgbColor Light::calculateFade(RgbColor fromColor, RgbColor toColor, uint8_t fromBrightness, uint8_t toBrightness, float progress) {
    return applyBrightness(RgbColor(
        fromColor.R + (float)(toColor.R - fromColor.R)*progress,
        fromColor.G + (float)(toColor.G - fromColor.G)*progress,
        fromColor.B + (float)(toColor.B - fromColor.B)*progress
    ),
    (fromBrightness + (float)(toBrightness - fromBrightness) * progress));
}

RgbColor Light::applyBrightness(RgbColor color, uint8_t brightness) {
  return RgbColor(
      color.R * brightness / 255,
      color.G * brightness / 255,
      color.B * brightness / 255
  );  
}

RgbColor Light::currentColor()
{
    RgbColor color;
    if (is_fading) {
        fade_progress += fade_step;
      
        if (fade_progress >= 1) {
            is_fading = false;
            color = applyBrightness(target_color, target_brightness);
        } else {
            color = calculateFade(prev_color, target_color, prev_brightness, target_brightness, fade_progress);
        }
    } else {
        color = applyBrightness(target_color, target_brightness);
    }
    return color;
}

RgbColor* Light::render()
{
    if (active_effect.equals("none")) {
        RgbColor color = currentColor();
        for (int i = 0; i < num_pixels; i++) {
            pixels[i] = color;
        }
    } else {
        int tick = millis() / 62 % 256;
        int brightness = target_brightness;
        
        if (is_fading) {
            fade_progress += fade_step;

            if (fade_progress >= 1) {
                is_fading = false;
                brightness = target_brightness;
            } else {
                brightness = (prev_brightness + (float)(target_brightness - prev_brightness) * fade_progress);
            }
        }
        
        rainbowCycle(pixels, num_pixels, tick, brightness);
    }
    return pixels;
}

const char* Light::prefixIt(const char* prefix, const char* str)
{
    char* value = (char*) malloc(strlen(prefix) + strlen(str));
    strcpy(value, prefix);
    strcat(value, str);
    return value;
}


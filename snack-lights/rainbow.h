#include <NeoPixelBus.h>
#include "Arduino.h"

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
RgbColor Wheel(byte WheelPos, byte brightness) {
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85) {
      return RgbColor(
          (255 - WheelPos * 3) * brightness / 255,
          0,
          (WheelPos * 3) * brightness / 255
      );
    } else if(WheelPos < 170) {
        WheelPos -= 85;
        return RgbColor(
            0,
            (WheelPos * 3) * brightness / 255,
            (255 - WheelPos * 3) * brightness / 255
        );
    } else {
        WheelPos -= 170;
        return RgbColor(
            (WheelPos * 3) * brightness / 255,
            (255 - WheelPos * 3) * brightness / 255,
            0
        );
    }
}

void rainbowCycle(RgbColor* pixel_data, int number_of_pixels, int rainbowTick, byte brightness) {
    uint16_t i, j;

    for (i = 0; i < number_of_pixels; i++) {
        pixel_data[i] = Wheel(((i * 256 / number_of_pixels) + rainbowTick) & 255, brightness);
    }
}

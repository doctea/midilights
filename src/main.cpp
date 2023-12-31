#include "Config.h"

#include <Arduino.h>
#include "Adafruit_NeoPXL8.h"

#include "midi_usb/midi_usb_rp2040.h"

#include "bpm.h"
#include "clock.h"

Adafruit_NeoPXL8 leds(NUM_PIXELS, pins, COLOR_ORDER);

// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  if (!leds.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) {
      Serial.println("no worky");
      digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
    }
  }

  setup_midi();
  setup_usb();

  leds.show(); // Clear initial LED state
}

uint32_t last_ticked = 0;
uint32_t ms_per_tick = 22;

float peak = 0.0;
//uint32_t ticks = 0;
void loop() {
  #ifdef USE_TINYUSB
      USBMIDI.read();
  #endif

  bool ticked = false;
  ticked = update_clock_ticks();

  if (ticked) {
    // fade global brightness value every tick
    peak -= peak/(float)(PEAK_DROP_RATE);
    if (peak < VAL_MINIMUM)
      peak = VAL_MINIMUM;

    if (is_bpm_on_phrase(ticks))
      peak = VAL_GLOBAL_ON_PHRASE;
    else if (is_bpm_on_bar(ticks))
      peak = VAL_GLOBAL_ON_BAR;
    else if (is_bpm_on_beat(ticks))
      peak = VAL_GLOBAL_ON_BEAT;

    //Serial.println("=====tick=====");
    for (int n = 0 ; n < NUM_PIXELS ; n++) {
      #ifdef LED_DIRECTION_REVERSE
        int i = num_pixels - n;
      #else
        int i = n;
      #endif
      
      // minimum brightness
      if (peak<VAL_MINIMUM)
        peak = VAL_MINIMUM;

      //float varhue = (float)i / (float)num_pixels;
      float varhue = (float)(ticks%(PPQN*BARS_PER_PHRASE*BEATS_PER_BAR)) / (float)(PPQN*BARS_PER_PHRASE*BEATS_PER_BAR);
      #ifndef LED_DIRECTION_REVERSE
        varhue = 1.0f - varhue;
      #endif
      float h = varhue + ((float)((ticks%(NUM_PIXELS)))/(float)NUM_PIXELS);
      float s = SAT_MINIMUM; // saturation initial
      //float h = varhue + ((float)num_pixels) / ((float)(ticks%num_pixels));

      // so that we can remember the global peak in order to smoother flashes
      float p = peak;
      // if we've just ticked and on a beat, make whole strip go bright with global brightness!
      /*if (ticked && is_bpm_on_beat(ticks)) {
        p = VAL_GLOBAL_ON_BEAT;
      }*/

      // if the current pixel corresponds to the current tick, make it bright
      if (ticks % NUM_PIXELS == i)
        p = VAL_CURRENT_TICK_PIXEL;

      //Serial.printf("pixel@\t%2i/%2i: hsv(%2.4f,\t%2.4f,\t%2.4f)\n", i, num_pixels, varhue, 1.0f, p);

      // set a slightly brighter colour proportional to proximity to the current beat marker
      float pixel_distance_from_beat = abs((int8_t)(ticks%NUM_PIXELS) - i);
      if(pixel_distance_from_beat==0) pixel_distance_from_beat = 0.5; // avoid division by zero
      bool after_current = (int8_t)(ticks%NUM_PIXELS) < i;
      //int pixel_distance_from_beat = (int8_t)(ticks%num_pixels) - i;
      if (!after_current && pixel_distance_from_beat > 0) { //} && pixel_distance_from_beat < 12) {
        s = constrain(s + ((SAT_MAXIMUM-SAT_MINIMUM) / (float)(pixel_distance_from_beat)), SAT_MINIMUM, SAT_MAXIMUM);
        p = constrain(p + ((VAL_MAXIMUM-VAL_MINIMUM) / (float)(pixel_distance_from_beat*2.0)), VAL_MINIMUM, VAL_MAXIMUM);
      } else if (after_current) { //} && pixel_distance_from_beat < 0) {
        //p = constrain(p / abs((float)pixel_distance_from_beat), VAL_MINIMUM, peak);
        //p /= (float)pixel_distance_from_beat;
        p = peak/2.0;
        s /= pixel_distance_from_beat;
      }

      // convert to final colours
      uint16_t hue = 65536.0 * h;
      int8_t sat = s * 255.0;
      int8_t val = p * 255.0;

      //Serial.printf("pixel@\t%2i/%2i: hsv(%i,\t%i,\t%i)\n", i, num_pixels, hue, sat, val);
      // do the actual setting
      uint32_t color = leds.ColorHSV(hue, sat, val); 
      leds.setPixelColor(i, color);
    }
    leds.show();
    //delay(1000);
  }

}

#include "Config.h"

#include <Arduino.h>
#include <SPI.h>

#include "Adafruit_TinyUSB.h"
#include "Adafruit_NeoPXL8.h"

#include "midi_usb/midi_usb_rp2040.h"

#include "bpm.h"
#include "clock.h"

Adafruit_NeoPXL8 *leds; //(NUM_PIXELS, pins, COLOR_ORDER);

light_mode_t mode = DEFAULT_LIGHT_MODE;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  if (mode==DEFAULT) {
    leds = new Adafruit_NeoPXL8(NUM_PIXELS, pins, COLOR_ORDER);
  } else if (mode==REPEATED || mode==REPEATED_REVERSE_SECOND) {
    leds = new Adafruit_NeoPXL8(NUM_PIXELS*2, pins, COLOR_ORDER);
  } else if (mode==DOUBLED_LENGTH) {
    NUM_PIXELS *= 2;
    leds = new Adafruit_NeoPXL8(NUM_PIXELS, pins, COLOR_ORDER);
  }

  if (!leds->begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) {
      Serial.println("no worky");
      digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
    }
  }

  setup_midi();
  setup_usb();

  leds->show(); // Clear initial LED state
}

float peak = 0.0;

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
      float varhue = (float)(ticks%(TICKS_PER_PHRASE)) / (float)(TICKS_PER_PHRASE);
      #ifndef LED_DIRECTION_REVERSE
        varhue = 1.0f - varhue;
      #endif
      float h = varhue + ((float)((ticks%(NUM_PIXELS)))/(float)NUM_PIXELS);
      float s = SAT_MINIMUM; // saturation initial
      //float h = varhue + ((float)num_pixels) / ((float)(ticks%num_pixels));

      // so that we can remember the global peak in order to smooth flashes
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
      //uint32_t color = leds->gamma32(leds->ColorHSV(hue, sat, val));  // hmmm gamma32 seems to wreck pixel brightness, but it actually is a kinda cool effect
      uint32_t color = leds->ColorHSV(hue, sat, val);  
      leds->setPixelColor(i, color);
      if (mode==REPEATED) {
        leds->setPixelColor(NUM_PIXELS + i, color);
      } else if (mode==REPEATED_REVERSE_SECOND) {
        leds->setPixelColor(NUM_PIXELS + (NUM_PIXELS - i), color);
      }
    }
    leds->show();
    //delay(1000);
  }

}

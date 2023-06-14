#define USE_TINYUSB

#include "Config.h"

#include <Arduino.h>
#include "Adafruit_NeoPXL8.h"

// MIDI + USB
//#ifdef USE_TINYUSB
//#include <Adafruit_TinyUSB.h>
//#endif
//#include <MIDI.h>

#include "midi/midi_usb.h"

#include "bpm.h"
#include "clock.h"

int8_t pins[8] = { 15, -1, -1, -1, -1 , -1, -1, -1 }; 
#define COLOR_ORDER NEO_RGB

#define num_pixels 96

Adafruit_NeoPXL8 leds(num_pixels, pins, COLOR_ORDER);

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
  // put your main code here, to run repeatedly:
  /*Serial.println("hello");
  delay(500);
  for (int i = 0 ; i < num_pixels ; i++) {
    leds.setPixelColor(i, random(255), random(255), random(255), random(255));
    leds.show();
  }*/

  #ifdef USE_TINYUSB
      USBMIDI.read();
  #endif

  bool ticked = false;
  ticked = update_clock_ticks();
  /*if (millis() - last_ticked > ms_per_tick) {
    last_ticked = millis();
    ticks++;
    ticked = true;
  }*/

  if (ticked) {
    peak /= 6;

    Serial.println("=====tick=====");
    for (int n = 0 ; n < num_pixels ; n++) {
      int i = num_pixels - n; // - 1;
      if (peak<0.1f)
        peak = 0.1f;

      float varhue = (float)i / (float)num_pixels;
      //float v = ((float)ticks/(float)num_pixels) + hue + 0.1;
      //float v = ((float)(ticks%96)/(float)num_pixels) + hue + 0.1f;
      float h = varhue + ((float)((ticks%96))/(float)num_pixels);

      float p = peak;
      if (ticked && is_bpm_on_beat(ticks))
        p = 1.0f;

      /*if (num_pixels - (ticks % 96) == i) {
        p = 0.1;
      }*/
      if (ticks % num_pixels == (num_pixels-i))
        p = 0.75f;
      //if (ticks%96)

      //Serial.printf("pixel@\t%2i/%2i: hsv(%2.4f,\t%2.4f,\t%2.4f)\n", i, num_pixels, varhue, 1.0f, p);

      uint16_t hue = 65536.0 * h;
      int8_t sat = 255;
      int8_t val = p * 255.0;

      //Serial.printf("pixel@\t%2i/%2i: hsv(%i,\t%i,\t%i)\n", i, num_pixels, hue, sat, val);

      uint32_t color = leds.ColorHSV(hue, sat, val); 

      leds.setPixelColor(i, color);
    }
    leds.show();
    //delay(1000);
  }

  if (ticked && is_bpm_on_beat(ticks) == 0) {
    if (is_bpm_on_bar(ticks))
      Serial.println("----BAR----!");
    Serial.printf("beat %i! (tick %i)\n", ticks / 24, ticks);
  }

}

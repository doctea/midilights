#include "Config.h"

#include <Arduino.h>
#include <SPI.h>

#include "Adafruit_TinyUSB.h"
#include "Adafruit_NeoPXL8.h"

#include "midi_usb/midi_usb_rp2040.h"

#include "bpm.h"
#include "clock.h"

#include "BootConfig.h"

Adafruit_NeoPXL8 *leds; //(NUM_PIXELS, pins, COLOR_ORDER);

light_mode_t mode = DEFAULT_LIGHT_MODE;


#define NUM_PIXELS_RGB 96
#define NUM_PIXELS_UV 60

void setup_parameter_inputs();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  #ifdef WAIT_FOR_SERIAL
    while (!Serial) {
      delay(100);
    }
  #endif
  Serial.println("STARTING UP!"); Serial.flush();
  

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
      Serial.println("no worky"); Serial.flush();
      digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
    }
  }

  Serial.println("setup_midi!"); Serial.flush();
  setup_midi();
  Serial.println("setup_usb!"); Serial.flush();
  setup_usb();

  Serial.println("setup_parameter_inputs!"); Serial.flush();
  setup_parameter_inputs();

  leds->show(); // Clear initial LED state
  Serial.println("finishing setup()!"); Serial.flush();

  // slow the whole shebang down
  set_bpm(1);
}


#include "ParameterManager.h"
#include "parameter_inputs/ParameterInput.h"
#include "parameter_inputs/VirtualParameterInput.h"

#include "parameter_inputs/VoltageParameterInput.h"
#include "voltage_sources/ADS24vVoltageSource.h"
#include "devices/ADCPimoroni24v.h"

ParameterManager *parameter_manager = nullptr;

//using InputType = VirtualParameterInput;
using InputType = VoltageParameterInput;

ADS1015 adcdevice(0x49,&Wire);

InputType *lfo1;
InputType *lfo2;
InputType *lfo3;
VirtualParameterInput *lfo4, *lfo5;

FloatParameter *p1;
FloatParameter *p2;
FloatParameter *p3;
FloatParameter *p4;

int p1_cursor, p2_cursor, p3_cursor, p4_cursor;
float p1_history[NUM_PIXELS_RGB];
float p2_history[NUM_PIXELS_RGB];
float p3_history[NUM_PIXELS_RGB];
float p4_history[NUM_PIXELS_UV];

void setup_parameter_inputs() {
  parameter_manager = new ParameterManager(TICKS_PER_PHRASE);
  parameter_manager->init();
  parameter_manager->debug = true;

  parameter_manager->addADCDevice(new ADCPimoroni24v(ENABLE_CV_INPUT, &Wire, 5.0));
  parameter_manager->auto_init_devices();

  /*lfo1 = new VirtualParameterInput("LFO1", "LFOs", LFO_LOCKED);
  lfo2 = new VirtualParameterInput("LFO2", "LFOs", LFO_FREE);
  lfo3 = new VirtualParameterInput("LFO3", "LFOs", LFO_LOCKED);*/
  lfo1 = (InputType*) parameter_manager->available_inputs->get(0);
  lfo2 = (InputType*) parameter_manager->available_inputs->get(1);
  lfo3 = (InputType*) parameter_manager->available_inputs->get(2);
  lfo4 = new VirtualParameterInput("LFO4", "LFOs", LFO_FREE);
  lfo5 = new VirtualParameterInput("LFO5", "LFOs", LFO_FREE);
  
  parameter_manager->addInput(lfo1);
  parameter_manager->addInput(lfo2);
  parameter_manager->addInput(lfo3);
  parameter_manager->addInput(lfo4);
  parameter_manager->addInput(lfo5);

  p1 = new FloatParameter("hue");
  p2 = new FloatParameter("sat");
  p3 = new FloatParameter("val");
  p4 = new FloatParameter("uv");

  parameter_manager->addParameter(p1);
  parameter_manager->addParameter(p2);
  parameter_manager->addParameter(p3);
  parameter_manager->addParameter(p4);

  /*p1->maximumNormalValue = 1.0;
  p1->minimumNormalValue = -0.5;
  p2->minimumNormalValue = -0.5;
  p3->minimumNormalValue = -0.5;
  p4->minimumNormalValue = -0.5;*/

  p1->connect_input(lfo1, 1.0);
  p1->connect_input(lfo2, -0.25);

  p2->connect_input(lfo2, 1.0);
  p2->connect_input(lfo3, -0.25);

  p3->connect_input(lfo3, 1.0);
  p3->connect_input(lfo4, -0.25);

  p4->connect_input(lfo4, 1.0);
  p4->connect_input(lfo1, -0.25);
  p4->connect_input(lfo2, 0.33);
  p4->connect_input(lfo3, -0.33);

  /*lfo1->locked_period = 4.0; //105; //4.0;
  lfo3->locked_period = 3.0; //170; //3.0;

  lfo2->locked_phase = 0.25;
  lfo2->free_sine_divisor = 105.0; //50.0;*/
  lfo4->locked_phase = 0.75;
  lfo4->free_sine_divisor = 3570.0; //150.0;
}


float peak = 0.0;

float hue = 0.0f;
float sat = 0.0f;
float val = 0.0f;
float uv = 0.0f;

void calculate_colours_chaser(float peak) {
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

      Serial.printf("pixel@\t% 2i/% 2i: hsv(% 5i,\t% 5i,\t% 5i)\n", i, NUM_PIXELS, hue, sat, val);
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
}


void calculate_colours(float peak) {
  int t = ticks / 100;
  /*
  hue = lfo1->get_normal_value_unipolar(); //(fmod(0.5 + asin(t), 1.0));
  sat = lfo2->get_normal_value_unipolar(); //(fmod(0.5 + acos(t), 1.0));
  val = lfo3->get_normal_value_unipolar(); //(fmod(0.5 + atan(t), 1.0));
  uv =  lfo4->get_normal_value_unipolar(); //(fmod(0.5 + asin(t), 1.0));
  */
  //hue = p1->getLastModulatedNormalValue();
  //sat = p2->getLastModulatedNormalValue();
  //val = p3->getLastModulatedNormalValue();
  uv  = p4->getLastModulatedNormalValue();
  /*hue = p1_history[n]; //p1_cursor % NUM_PIXELS_RGB];
  sat = p2_history[n]; //p1_cursor % NUM_PIXELS_RGB];
  val = p3_history[n]; //p1_cursor % NUM_PIXELS_RGB];
  uv = p4_history[n]; //p1_cursor % NUM_PIXELS_RGB];*/

  for (int n = 0 ; n < NUM_PIXELS_RGB ; n++) {
    float pc = (float)n/(float)NUM_PIXELS_RGB;
    hue = lfo1->get_normal_value_unipolar() * fmod(p1_history[(n+ticks)%NUM_PIXELS_RGB]*5.0,1.0); 
    sat = constrain(lfo2->get_normal_value_unipolar() * p2_history[n], 0.8, 1.0); 
    val = lfo3->get_normal_value_unipolar() * p3_history[(n-ticks)%NUM_PIXELS_RGB]; 

    uint32_t color = leds->ColorHSV(65535.0*hue, 255.0*sat, 255.0*val);  
    //Serial.printf("tick %5i :: calculate_colours: [% 3i/% 3i] => % 3f, % 3f, % 3f\n", t, n+1, NUM_PIXELS_RGB, hue, sat, val);
    leds->setPixelColor(n, color);
  }
  for (int n = 0 ; n < NUM_PIXELS_UV ; n++) {
    float pc = (float)n/(float)60;
    uv = p4_history[n] * pc;// * p1->getLastModulatedNormalValue(); 
    //uint32_t color = leds->ColorHSV(65535.0*uv, 255.0*uv, 255.0*uv);  
    leds->setPixelColor(NUM_PIXELS_RGB + n, uv*255.0, uv*255.0, uv*255.0);
  }
}

void loop() {
  #ifdef USE_TINYUSB
    USBMIDI.read();
  #endif

  bool ticked = false;
  ticked = update_clock_ticks();
  if (ticked) Serial.println("ticked!");

  //parameter_manager->throttled_update_cv_input__all();
  //parameter_manager->update_inputs();

  /*if (ticked) {
    //parameter_manager->update_inputs();
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

    //if (BPM_CURRENT_BAR_OF_PHRASE % 2 == 0)
    //  calculate_colours_chaser(peak);
    //else
      calculate_colours(peak);

    leds->show();
    //delay(1000);
  }*/


  if (false && ticked) {
    p1_history[++p1_cursor % NUM_PIXELS_RGB] = p1->getLastModulatedNormalValue();
    p2_history[++p2_cursor % NUM_PIXELS_RGB] = p2->getLastModulatedNormalValue();
    p3_history[++p3_cursor % NUM_PIXELS_RGB] = p3->getLastModulatedNormalValue();
    p4_history[++p4_cursor % NUM_PIXELS_UV]  = p4->getLastModulatedNormalValue();

    /*++p1_cursor;
    ++p2_cursor;
    ++p3_cursor;
    ++p4_cursor;*/
    
    /*
    p1_history[constrain((int)(lfo5->get_normal_value_unipolar() * NUM_PIXELS_RGB),0,NUM_PIXELS_RGB-1)] *= p1->getLastModulatedNormalValue();
    p2_history[constrain((int)(lfo5->get_normal_value_unipolar() * NUM_PIXELS_RGB),0,NUM_PIXELS_RGB-1)] *= p2->getLastModulatedNormalValue();
    p3_history[constrain(((int)lfo5->get_normal_value_unipolar() * NUM_PIXELS_RGB),0,NUM_PIXELS_RGB-1)] *= p3->getLastModulatedNormalValue();
    p4_history[constrain(((int)lfo5->get_normal_value_unipolar() * NUM_PIXELS_UV),0,NUM_PIXELS_UV-1)]  *= p4->getLastModulatedNormalValue();
    */

  }

  set_bpm(10.0 + pow(100.0*(1.0-p4->getLastModulatedNormalValue()), 0.25+p1->getLastModulatedNormalValue()*2.0));

  calculate_colours(peak);
  leds->show();
}

#ifndef CONFIG__INCLUDED
#define CONFIG__INCLUDED

#include <Arduino.h>

//#define NUM_PIXELS 96
int NUM_PIXELS = 96;

enum light_mode_t {
  DEFAULT,      // 'standard' mode of NUM_PIXELS elements per bar
  REPEATED,     // NUM_PIXELS elements per bar, but repeated for a total of NUM_PIXELS * 2 
  REPEATED_REVERSE_SECOND,  // NUM_PIXELS elements per bar, but repeated for a total of NUM_PIXELS * 2, with second set reversed
  DOUBLED_LENGTH
};
//#define DEFAULT_LIGHT_MODE = light_mode_t::DEFAULT;
//#define DEFAULT_LIGHT_MODE (light_mode_t::REPEATED)
#define DEFAULT_LIGHT_MODE (light_mode_t::REPEATED_REVERSE_SECOND)
//#define DEFAULT_LIGHT_MODE = light_mode_t::DOUBLED_LENGTH;

//#define LED_DIRECTION_REVERSE
#define PEAK_DROP_RATE          (PPQN/8)

#define VAL_GLOBAL_ON_PHRASE    1.0f
#define VAL_GLOBAL_ON_BAR       0.9f
#define VAL_GLOBAL_ON_BEAT      0.75f
#define VAL_CURRENT_TICK_PIXEL  1.0f //0.75f
#define VAL_MINIMUM             0.01f
#define VAL_MAXIMUM             1.0f

#define SAT_MINIMUM             0.75f
#define SAT_MAXIMUM             1.0f

int8_t pins[8] = { PIN_NEOPIXEL, -1, -1, -1, -1 , -1, -1, -1 }; 
#define COLOR_ORDER NEO_RGB

#endif
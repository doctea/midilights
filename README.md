# midilights

rgb led indicator of midi clock etc

## Requirements

- Suitable RP2040 MCU board with NeoPixel/WS281x RGB pixel driver - I'm using a Pimoroni Plasma 2040, but any RP2040 should work fine with the appropriate circuitry to drive the pixels.
- Suitable pixel strip (I'm using the [Neon-like RGB LED Strip with Diffuser](https://shop.pimoroni.com/products/neon-like-rgb-led-strip-with-diffuser-neopixel-ws2812-sk6812-compatible?variant=39346118262867) which has 96 pixels, perfect for indicating 4 beats of MIDI clock)
- [https://github.com/doctea/midihelpers](midihelpers) library for MIDI clock timing and start/stop/reset
- ([https://github.com/doctea/parameters](parameters) library if/when I add CV indication)
- [NeoPXL8](https://github.com/adafruit/Adafruit_NeoPXL8) library for fast, non-blocking pixel strip updates
- PlatformIO/VScode
- The earlephilhower Arduino-Pico core

## TODO

- Use the Pimoroni +/-24v ADC board to indicate (ie control the LEDs) via CV
- Different modes, eg
 - Instead of indicating MIDI clock, indicate different drum hits on channel 10
 - Instead of indicating 4 beats per strip, indicate a whole phrase of 16 beats
 - Control multiple strips simultaneously
- ???

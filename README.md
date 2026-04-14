# Traffic Light Toy

A small Raspberry Pi Pico toy that I built for my daughter, who has been obsessed with traffic lights lately.

# What it does

This toy simulates what it feels like to wait at a traffic light.

When something is detected close to the sensor, it triggers a sequence:

Red (random wait, like a real light)

Yellow (short pause)


Green (go)
Then everything turns off again

If nothing is detected, it just sits idle.

# Hardware

Raspberry Pi Pico (RP2040)

VL53L1X distance sensor

3 LEDs (red / yellow / green)

# Notes

Getting stable behaviour took a bit more work than expected. The sensor readings are noisy, so I added:

- hysteresis thresholds
- consecutive-read checks before triggering

This avoids flickering and false triggers and makes the interaction feel more deliberate.

The core logic lives in main.c.

# Build

mkdir build

cd build

cmake ..

make

Flash the .uf2 file to the Pico.

# Traffic Light Toy

A small Raspberry Pi Pico toy that I built for my daughter, who has been obsessed with traffic lights lately.

Bring an object close to the distance sensor and the toy cycles through red, yellow and green. Written in C, the firmware combines sensor input, timed LED control and checks to reduce false triggers.

## What it does

| Light | Duration |
| --- | --- |
| Red | Random wait between 2 and 8 seconds |
| Yellow | 5 seconds |
| Green | 10 seconds |

All LEDs turn off at the end of each sequence. If the next valid reading still keeps the sensor in its near state, the sequence repeats. Otherwise, the toy returns to idle.

The sequence uses blocking delays: the firmware does not read the sensor while the lights are cycling. Moving an object away does not interrupt a sequence already in progress.

## Hardware and wiring

- Raspberry Pi Pico (RP2040)
- VL53L1X time-of-flight distance sensor module
- Red, yellow and green LEDs, each with a suitable current-limiting resistor
- Connecting wires and a USB data cable

The firmware uses these **GPIO numbers**, not physical header pin numbers:

| Connection | Pico GPIO |
| --- | --- |
| Sensor SDA | GP4 (I2C0 SDA) |
| Sensor SCL | GP5 (I2C0 SCL) |
| Red LED | GP15 |
| Yellow LED | GP18 |
| Green LED | GP16 |

LED outputs are active-high: connect each output through a suitable resistor to its LED anode, with the cathode connected to ground. Use a common ground for the Pico and sensor. Follow your sensor module's power requirements and ensure its I2C signals are compatible with the Pico's 3.3 V GPIO. The repository does not specify the exact breakout board or resistor values.

The sensor uses the 7-bit I2C address `0x29`. The application configures I2C at 100 kHz.

## Handling noisy readings

Separate entry and exit distances prevent small changes around one threshold from repeatedly switching detection:

- **Enter the near state:** accumulate 10 valid readings at or below 145 mm. A valid reading above 145 mm resets the entry count.
- **Leave the near state:** a valid reading at or above 155 mm resets detection.
- **Invalid readings:** five consecutive invalid range measurements reset detection and leave the LEDs off. Fewer than five invalid readings preserve the current entry count.

These settings are near the top of [`src/main.c`](src/main.c). The variable `green_state` represents near detection, rather than which LED is currently lit.

## Repository structure

- [`src/main.c`](src/main.c): startup, sensor polling, detection checks and light sequences.
- [`lib/vl53l1x/`](lib/vl53l1x/): bundled sensor driver and Pico I2C integration.
- [`CMakeLists.txt`](CMakeLists.txt): firmware sources, SDK libraries and build target.
- [`pico_sdk_import.cmake`](pico_sdk_import.cmake): Pico SDK discovery.

The bundled driver files retain their upstream copyright and licence notices.

## Build

Install Git, CMake, Python 3, an Arm embedded GCC toolchain (`arm-none-eabi-gcc`), a native C/C++ compiler and a supported build tool such as Make or Ninja. Follow the [official Raspberry Pi C/C++ setup guide](https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html) for your operating system.

Clone the project and SDK into sibling directories:

```sh
git clone https://github.com/yunkiyau/traffic-light-toy.git
git clone --recurse-submodules https://github.com/raspberrypi/pico-sdk.git
cd traffic-light-toy
cmake -S . -B build -DPICO_SDK_PATH=../pico-sdk -DPICO_BOARD=pico
cmake --build build
```

If the SDK is already installed, replace `../pico-sdk` with its path. For Ninja, add `-G Ninja` to the configure command and ensure Ninja is installed. On Windows, use a terminal with the Pico toolchain configured.

The output is `build/traffic_light_final.uf2`. The build target is named `traffic_light_final`, even though the repository is named `traffic-light-toy`. SDK and toolchain versions are not pinned in this repository.

## Flash the Pico

1. Disconnect the Pico from USB.
2. Hold **BOOTSEL** while connecting it with a USB data cable, then release the button.
3. Copy `build/traffic_light_final.uf2` onto the `RPI-RP2` drive.
4. The Pico restarts and runs the firmware.

At startup, red lights for one second, yellow blinks while the sensor boots, and all three LEDs light for one second when initialisation completes. The LEDs then turn off until a nearby object is detected.

## Checks and troubleshooting

Let startup finish, bring an object within 145 mm, and hold it there long enough for the entry checks. Confirm the red/yellow/green sequence, then move the object beyond 155 mm and wait for the current sequence to finish.

- **Red repeatedly blinks:** the firmware entered its error loop after a sensor communication, initialisation or data-ready error. Check power, ground, SDA/SCL wiring and sensor compatibility, then restart the Pico.
- **No trigger:** check distance and sensor alignment. Triggering requires repeated valid readings, not a single brief pass.
- **No USB drive in BOOTSEL mode:** check that the USB cable supports data.
- **SDK not found when configuring:** check that `PICO_SDK_PATH` points to the SDK directory.

These are manual checks for the hardware; the repository does not include an automated hardware test suite.

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "VL53L1X_api.h"
#include "VL53L1X_platform.h"

#define GREEN_LED  16
#define YELLOW_LED 18
#define RED_LED    15

#define I2C_PORT i2c0
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5

#define TOF_ADDR  0x29   // 7-bit address (matches your fixed platform.c)

// Hysteresis thresholds (mm)
#define GREEN_ON_MM   145
#define GREEN_OFF_MM  155

// Only turn all LEDs off after this many consecutive invalid measurements
#define INVALID_OFF_REQUIRED 5

// Only enter green_state after this many consecutive "inside GREEN_ON_MM" reads
#define GREEN_ON_REQUIRED 10

static void leds_init(void) {
    gpio_init(RED_LED);    gpio_set_dir(RED_LED, GPIO_OUT);
    gpio_init(YELLOW_LED); gpio_set_dir(YELLOW_LED, GPIO_OUT);
    gpio_init(GREEN_LED);  gpio_set_dir(GREEN_LED, GPIO_OUT);
    gpio_put(RED_LED, 0);
    gpio_put(YELLOW_LED, 0);
    gpio_put(GREEN_LED, 0);
}

static void set_leds(bool r, bool y, bool g) {
    gpio_put(RED_LED, r ? 1 : 0);
    gpio_put(YELLOW_LED, y ? 1 : 0);
    gpio_put(GREEN_LED, g ? 1 : 0);
}

static void all_off(void) {
    set_leds(false, false, false);
}

static void error_blink_red(void) {
    while (true) {
        gpio_put(RED_LED, 1); sleep_ms(200);
        gpio_put(RED_LED, 0); sleep_ms(200);
    }
}

// Run the requested sequence, ending with all LEDs OFF
static void run_valid_sequence(void) {

    // Random RED duration between 2–8 seconds
    uint32_t red_duration_ms = (rand() % 6001) + 2000;  
    // 0–6000 → +2000 gives 2000–8000 ms

    // RED on random duration then off
    set_leds(true, false, false);
    sleep_ms(red_duration_ms);
    all_off();

    // YELLOW on 5s then off
    set_leds(false, true, false);
    sleep_ms(5000);
    all_off();

    // GREEN on 10s then off
    set_leds(false, false, true);
    sleep_ms(10000);
    all_off();
}

int main() {
    
    // Seed RNG using hardware timer
    srand(to_us_since_boot(get_absolute_time()));

    leds_init();

    // Stage 0: firmware alive (1s red)
    gpio_put(RED_LED, 1);
    sleep_ms(1000);
    gpio_put(RED_LED, 0);

    // I2C init (safe even though platform also does it)
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    sleep_ms(50);

    uint16_t dev = TOF_ADDR;

    // Wait for boot (blink yellow while waiting)
    uint8_t booted = 0;
    absolute_time_t boot_t0 = get_absolute_time();
    while (!booted) {
        gpio_put(YELLOW_LED, 1); sleep_ms(50);
        gpio_put(YELLOW_LED, 0); sleep_ms(50);

        if (VL53L1X_BootState(dev, &booted) != 0) error_blink_red();
        if (absolute_time_diff_us(boot_t0, get_absolute_time()) > 3000000) error_blink_red();
    }

    // Init + config
    if (VL53L1X_SensorInit(dev) != 0) error_blink_red();
    if (VL53L1X_SetDistanceMode(dev, 2) != 0) error_blink_red();          // 2 = long
    if (VL53L1X_SetTimingBudgetInMs(dev, 200) != 0) error_blink_red();
    if (VL53L1X_SetInterMeasurementInMs(dev, 220) != 0) error_blink_red();
    if (VL53L1X_StartRanging(dev) != 0) error_blink_red();

    // "Ready" indicator: all LEDs on for 1s
    set_leds(true, true, true);
    sleep_ms(1000);

    // After startup: all LEDs OFF by default
    all_off();

    int invalid_streak = 0;
    absolute_time_t first_data_t0 = get_absolute_time();

    // Hysteresis state: remembers whether we're currently "in green"
    bool green_state = false;

    // Require N consecutive <= GREEN_ON_MM to enter green_state
    int green_on_streak = 0;

    // Reset the sequence flag after each run to allow repeats while near
    bool sequence_ran_this_episode = false;

    while (true) {
        uint8_t ready = 0;
        if (VL53L1X_CheckForDataReady(dev, &ready) != 0) error_blink_red();

        if (!ready) {
            // No new measurement yet -> keep whatever LEDs are currently doing
            if (absolute_time_diff_us(first_data_t0, get_absolute_time()) > 2000000) {
                error_blink_red();
            }
            sleep_ms(5);
            continue;
        }

        first_data_t0 = get_absolute_time();

        uint16_t distance_mm = 0;
        uint8_t rangeStatus = 0;

        if (VL53L1X_GetDistance(dev, &distance_mm) != 0) error_blink_red();
        if (VL53L1X_GetRangeStatus(dev, &rangeStatus) != 0) error_blink_red();
        if (VL53L1X_ClearInterrupt(dev) != 0) error_blink_red();

        if (rangeStatus == 0) {
            invalid_streak = 0;

            // ----- GREEN-ENTRY STREAK + HYSTERESIS -----
            if (!green_state) {
                if (distance_mm <= GREEN_ON_MM) {
                    green_on_streak++;
                    if (green_on_streak >= GREEN_ON_REQUIRED) {
                        green_state = true;
                        sequence_ran_this_episode = false; // new episode starts now
                    }
                } else {
                    green_on_streak = 0;
                }
            } else {
                // Leave green only when >= GREEN_OFF_MM
                if (distance_mm >= GREEN_OFF_MM) {
                    green_state = false;
                    green_on_streak = 0;
                    sequence_ran_this_episode = false;
                    all_off(); // while not green_state, stay OFF
                }
            }

            // Sequence behavior:
            // - If not green_state yet: all LEDs OFF (no yellow indicator).
            // - In green_state: run the sequence, ending with all LEDs OFF.
            // - After the next valid reading, repeat if still in green_state.
            // - Sensor reads pause during the blocking sequence.
            if (!green_state) {
                all_off();
            } else {
                if (!sequence_ran_this_episode) {
                    sequence_ran_this_episode = true;
                    run_valid_sequence();          // ends OFF
                    sequence_ran_this_episode = false; // allow repeat while still green_state
                } else {
                    all_off();
                }

            }

        } else {
            // Invalid measurement: after N consecutive invalids, declare "no target" and go OFF
            invalid_streak++;
            if (invalid_streak >= INVALID_OFF_REQUIRED) {
                all_off();
                green_state = false;
                green_on_streak = 0;
                sequence_ran_this_episode = false;
            } else {
                // If you truly want OFF immediately on ANY invalid read, uncomment this:
                // all_off();
            }
        }

        sleep_ms(20);
    }
}

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

static void show_out_of_range_yellow(void) {
    set_leds(false, true, false);   // yellow only
}

static void show_in_range_green(void) {
    set_leds(false, false, true);   // green only
}

static void error_blink_red(void) {
    while (true) {
        gpio_put(RED_LED, 1); sleep_ms(200);
        gpio_put(RED_LED, 0); sleep_ms(200);
    }
}

int main() {
    stdio_init_all();
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

    // New: require N consecutive <= GREEN_ON_MM to enter green_state
    int green_on_streak = 0;

    while (true) {
        uint8_t ready = 0;
        if (VL53L1X_CheckForDataReady(dev, &ready) != 0) error_blink_red();

        if (!ready) {
            // No new measurement yet -> keep last LED state (prevents cadence flicker)
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

        // Only update LEDs on valid measurements.
        // If invalid, only turn off after INVALID_OFF_REQUIRED consecutive invalids.
        if (rangeStatus == 0) {
            invalid_streak = 0;

            // -------- GREEN-ENTRY STREAK + HYSTERESIS --------
            if (!green_state) {
                if (distance_mm <= GREEN_ON_MM) {
                    green_on_streak++;
                    if (green_on_streak >= GREEN_ON_REQUIRED) {
                        green_state = true;
                    }
                } else {
                    green_on_streak = 0; // must be consecutive inside threshold
                }
            } else {
                // Once green, we use the OFF hysteresis threshold to leave green
                if (distance_mm >= GREEN_OFF_MM) {
                    green_state = false;
                    green_on_streak = 0; // reset so re-entry still needs GREEN_ON_REQUIRED
                }
            }

            if (green_state) show_in_range_green();
            else             show_out_of_range_yellow();

        } else {
            invalid_streak++;
            if (invalid_streak >= INVALID_OFF_REQUIRED) {
                all_off();
                green_state = false;     // reset hysteresis state when we declare "no target"
                green_on_streak = 0;     // also reset the entry streak
            }
            // otherwise keep last LED state
        }

        sleep_ms(20);
    }
}

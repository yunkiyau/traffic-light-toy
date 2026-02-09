#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define GREEN_LED 16
#define YELLOW_LED 18
#define RED_LED 15

#define I2C_PORT i2c0
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define VL53L1X_ADDR 0x29

static bool tof_ack_at_0x29(void) {
    // Conservative I2C speed
    i2c_init(I2C_PORT, 100 * 1000);

    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);

    // Enable Pico internal pull-ups (safe even if breakout already has them)
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    sleep_ms(10); // allow bus/sensor to settle

    uint8_t dummy = 0;
    int ret = i2c_read_blocking(I2C_PORT, VL53L1X_ADDR, &dummy, 1, false);

    return (ret == 1);
}

int main() {
    stdio_init_all();

    gpio_init(GREEN_LED);
    gpio_set_dir(GREEN_LED, GPIO_OUT);

    gpio_init(YELLOW_LED);
    gpio_set_dir(YELLOW_LED, GPIO_OUT);

    gpio_init(RED_LED);
    gpio_set_dir(RED_LED, GPIO_OUT);

    bool ack = tof_ack_at_0x29();

    if (ack) {
        // Sensor ACKed: all LEDs on solid
        gpio_put(RED_LED, 1);
        gpio_put(YELLOW_LED, 1);
        gpio_put(GREEN_LED, 1);

        while (true) {
            sleep_ms(1000);
        }
    } else {
        // No ACK: blink red forever
        while (true) {
            gpio_put(RED_LED, 1);
            sleep_ms(250);
            gpio_put(RED_LED, 0);
            sleep_ms(250);
        }
    }
}

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define GREEN_LED 16
#define YELLOW_LED 18
#define RED_LED 15

#define I2C_PORT i2c0
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define VL53L1X_ADDR 0x29

// Initialise I2C once
static void i2c_setup(void) {
    i2c_init(I2C_PORT, 100 * 1000);

    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);

    // Safe to enable even if breakout already has pull-ups
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    sleep_ms(10); // allow bus + sensor to settle
}

// Read VL53L1X model ID register (0x010F)
static bool vl53l1x_read_model_id(uint8_t *model_id) {
    uint8_t reg_addr[2] = {0x01, 0x0F};

    // Write 16-bit register address, keep bus active
    if (i2c_write_blocking(I2C_PORT, VL53L1X_ADDR, reg_addr, 2, true) != 2) {
        return false;
    }

    // Read single byte from that register
    if (i2c_read_blocking(I2C_PORT, VL53L1X_ADDR, model_id, 1, false) != 1) {
        return false;
    }

    return true;
}

int main() {
    stdio_init_all();

    gpio_init(GREEN_LED);
    gpio_set_dir(GREEN_LED, GPIO_OUT);

    gpio_init(YELLOW_LED);
    gpio_set_dir(YELLOW_LED, GPIO_OUT);

    gpio_init(RED_LED);
    gpio_set_dir(RED_LED, GPIO_OUT);

    i2c_setup();

    uint8_t model_id = 0;
    bool ok = vl53l1x_read_model_id(&model_id);

    if (ok && model_id == 0xEA) {
        // ✅ Correct model ID read
        gpio_put(GREEN_LED, 1);
        gpio_put(YELLOW_LED, 1);
        gpio_put(RED_LED, 0);
    } 
    else if (ok) {
        // ⚠️ Register read worked, but ID unexpected
        gpio_put(GREEN_LED, 1);
        gpio_put(YELLOW_LED, 1);
        gpio_put(RED_LED, 1);
    } 
    else {
        // ❌ Register read failed
        while (true) {
            gpio_put(YELLOW_LED, 1);
            sleep_ms(250);
            gpio_put(YELLOW_LED, 0);
            sleep_ms(250);
        }
    }

    while (true) {
        sleep_ms(1000);
    }
}

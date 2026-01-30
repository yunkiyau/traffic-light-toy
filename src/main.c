#include "pico/stdlib.h"

#define GREEN_LED 16
#define YELLOW_LED 18
#define RED_LED 15

int main() {
    stdio_init_all();

    gpio_init(GREEN_LED);
    gpio_set_dir(GREEN_LED, GPIO_OUT);

    gpio_init(YELLOW_LED);
    gpio_set_dir(YELLOW_LED, GPIO_OUT);

    gpio_init(RED_LED);
    gpio_set_dir(RED_LED, GPIO_OUT);



    while (true) {
        gpio_put(GREEN_LED, 1);
        gpio_put(YELLOW_LED, 1);
        gpio_put(RED_LED, 1);

        sleep_ms(250);

        gpio_put(GREEN_LED, 0);
        gpio_put(YELLOW_LED, 0);
        gpio_put(RED_LED, 0);

        sleep_ms(250);
    }
}

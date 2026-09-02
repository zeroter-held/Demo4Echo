#include "gpio_manager.h"

#include <stdio.h>

static int gpio_values[256];

int calculate_gpio_pin(int bank, int group, int x)
{
    return bank * 32 + (group * 8 + x);
}

int gpio_export(int gpio_pin)
{
    printf("[SIM] gpio export %d\n", gpio_pin);
    return 0;
}

int gpio_unexport(int gpio_pin)
{
    printf("[SIM] gpio unexport %d\n", gpio_pin);
    return 0;
}

int gpio_set_direction(int gpio_pin, const char *direction)
{
    printf("[SIM] gpio %d direction=%s\n", gpio_pin, direction ? direction : "(null)");
    return 0;
}

int gpio_set_value(int gpio_pin, int value)
{
    if(gpio_pin >= 0 && gpio_pin < (int)(sizeof(gpio_values) / sizeof(gpio_values[0]))) {
        gpio_values[gpio_pin] = value;
    }
    return 0;
}

int gpio_get_value(int gpio_pin)
{
    if(gpio_pin < 0 || gpio_pin >= (int)(sizeof(gpio_values) / sizeof(gpio_values[0]))) return -1;
    return gpio_values[gpio_pin];
}

void gpio_init(int gpio_pin, const char *direction)
{
    gpio_export(gpio_pin);
    gpio_set_direction(gpio_pin, direction);
}

void gpio_deinit(int gpio_pin)
{
    gpio_unexport(gpio_pin);
}

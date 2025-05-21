#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "pico/bootrom.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "lib/ssd1306/ssd1306.h"
#include "lib/ssd1306/display.h"
#include "lib/led/led.h"
#include "lib/button/button.h"
#include "lib/ws2812b/ws2812b.h"
#include "lib/buzzer/buzzer.h"

void gpio_irq_handler(uint gpio, uint32_t events); // Função de interrupção para o botão B

int main()
{
    // Ativa BOOTSEL via botão B
    init_btn(BTN_B_PIN);
    gpio_set_irq_enabled_with_callback(BTN_B_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    stdio_init_all();

    // Inicia o agendador
    vTaskStartScheduler();
    panic_unsupported();
}

// Função de interrupção para o botão B
void gpio_irq_handler(uint gpio, uint32_t events)
{
    reset_usb_boot(0, 0);
}
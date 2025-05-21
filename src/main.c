#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "pico/bootrom.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "lib/ssd1306/ssd1306.h"
#include "lib/ssd1306/display.h"
#include "lib/led/led.h"
#include "lib/button/button.h"
#include "lib/buzzer/buzzer.h"

ssd1306_t ssd;
const uint8_t MAX = 8;
SemaphoreHandle_t xMutex;
static volatile int available_tokens = MAX;

void vTaskEntrance(void *params);
void vTaskExit(void *params);
void vTaskReset(void *params);
void gpio_irq_handler(uint gpio, uint32_t events); // Função de interrupção para o botão B

int main()
{
    stdio_init_all();

    // Ativa BOOTSEL via botão SW
    init_btn(BTN_SW_PIN);
    init_display(&ssd);

    gpio_set_irq_enabled_with_callback(BTN_SW_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Inicializa o Mutex
    xMutex = xSemaphoreCreateMutex();

    xTaskCreate(vTaskEntrance, "Task de entrada", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskExit, "Task de saída", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskReset, "Task de reset", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);

    // Inicia o agendador
    vTaskStartScheduler();
    panic_unsupported();
}

// Função de interrupção para o botão SW
void gpio_irq_handler(uint gpio, uint32_t events)
{
    reset_usb_boot(0, 0);
}

void vTaskEntrance(void *params) {
    init_btn(BTN_A_PIN);
    char buffer[40];

    while(true) {
        if (xSemaphoreTake(xMutex, portMAX_DELAY)==pdTRUE) {
            ssd1306_fill(&ssd, false);
            draw_centered_text(&ssd, "Fichas RU", 3);

            snprintf(buffer, sizeof(buffer), "Total: %d", MAX);
            ssd1306_draw_string(&ssd, buffer, 3, 25);
            snprintf(buffer, sizeof(buffer), "Livres: %d", available_tokens);
            ssd1306_draw_string(&ssd, buffer, 3, 36);

            ssd1306_send_data(&ssd);
            xSemaphoreGive(xMutex);
        }
        vTaskDelay(pdTICKS_TO_MS(3000));
    };
}

void vTaskExit(void *params) {
    init_btn(BTN_B_PIN);
    char buffer[40];

    while(true) {
        if (xSemaphoreTake(xMutex, portMAX_DELAY)==pdTRUE) {
            ssd1306_fill(&ssd, false);
            draw_centered_text(&ssd, "Fichas RU", 3);

            snprintf(buffer, sizeof(buffer), "Total: %d", MAX);
            ssd1306_draw_string(&ssd, buffer, 3, 25);
            snprintf(buffer, sizeof(buffer), "Livres: %d", available_tokens);
            ssd1306_draw_string(&ssd, buffer, 3, 36);

            ssd1306_send_data(&ssd);
            xSemaphoreGive(xMutex);
        }
        vTaskDelay(pdTICKS_TO_MS(2000));
    };
}

void vTaskReset(void *params) {
    while(true) {
        printf("Tarefa de reset.\n");
        vTaskDelay(pdTICKS_TO_MS(1000));
    };
}
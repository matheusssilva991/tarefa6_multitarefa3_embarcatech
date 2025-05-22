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
SemaphoreHandle_t xCounterSemphr;
SemaphoreHandle_t xBtnSwSemphr;
SemaphoreHandle_t xBtnASemphr;
SemaphoreHandle_t xBtnBSemphr;
static volatile int avaliable_tokens = MAX;
static volatile int64_t last_time_btn_a = 0;
static volatile int64_t last_time_btn_b = 0;

void vTaskEntrance(void *params);
void vTaskExit(void *params);
void vTaskReset(void *params);
void update_display();
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
    xCounterSemphr = xSemaphoreCreateCounting(MAX, avaliable_tokens);
    xBtnSwSemphr = xSemaphoreCreateBinary();
    xBtnASemphr = xSemaphoreCreateBinary();
    xBtnBSemphr = xSemaphoreCreateBinary();

    xTaskCreate(vTaskEntrance, "Task de entrada", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskExit, "Task de saída", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    //xTaskCreate(vTaskReset, "Task de reset", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);

    // Inicia o agendador
    vTaskStartScheduler();
    panic_unsupported();
}

// Função de interrupção para o botão SW
void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint64_t current_time = to_ms_since_boot(get_absolute_time());

    if (gpio == BTN_SW_PIN) {
        reset_usb_boot(0, 0);

    } else if (gpio == BTN_A_PIN && current_time - last_time_btn_a > 270) {
        last_time_btn_a = current_time;
        xSemaphoreGiveFromISR(xBtnASemphr, NULL);
    } else if (gpio == BTN_B_PIN && current_time - last_time_btn_b > 270) {
        last_time_btn_b = current_time;
        xSemaphoreGiveFromISR(xBtnBSemphr, NULL);
    }


}

void vTaskEntrance(void *params) {
    init_btn(BTN_A_PIN);
    gpio_set_irq_enabled_with_callback(BTN_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Garante que o display seja atualizado na inicialização
    // Aguarda o mutex para atualizar o display
    if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
        update_display();
        xSemaphoreGive(xMutex);
    }

    while(true) {
        // Aguarda o botão A ser pressionado
        if (xSemaphoreTake(xBtnASemphr, portMAX_DELAY) == pdTRUE) {
            // Aguarda o semáforo de contagem
            if (xSemaphoreTake(xCounterSemphr, portMAX_DELAY) == pdTRUE) {
                avaliable_tokens--;

                if (avaliable_tokens < 0) {
                    avaliable_tokens = 0;
                }

                // Se o semáforo foi obtido, atualiza o display
                if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
                    update_display();
                    xSemaphoreGive(xMutex);
                }
            }
        }

        vTaskDelay(pdTICKS_TO_MS(150));
    };
}

void vTaskExit(void *params) {
    init_btn(BTN_B_PIN);
    gpio_set_irq_enabled_with_callback(BTN_B_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    while(true) {
        // Aguarda o botão B ser pressionado
        if (xSemaphoreTake(xBtnBSemphr, portMAX_DELAY) == pdTRUE) {
            // Libera um token
            if (xSemaphoreGive(xCounterSemphr) == pdTRUE) {
                avaliable_tokens++;

                if (avaliable_tokens > MAX) {
                    avaliable_tokens = MAX;
                }

                // Se o semáforo foi obtido, atualiza o display
                if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
                    update_display();
                    xSemaphoreGive(xMutex);
                }
            }
        }

        vTaskDelay(pdTICKS_TO_MS(150));
    };
}

void vTaskReset(void *params) {
    while(true) {
        vTaskDelay(pdTICKS_TO_MS(1000));
    };
}

void update_display() {
    char buffer[40];

    ssd1306_fill(&ssd, false);
    ssd1306_rect(&ssd, 0, 0, ssd.width, ssd.height, true, false);
    draw_centered_text(&ssd, "Fichas RU", 5);

    snprintf(buffer, sizeof(buffer), "Total: %d", MAX);
    ssd1306_draw_string(&ssd, buffer, 5, 25);
    snprintf(buffer, sizeof(buffer), "Livres: %d",  avaliable_tokens);
    ssd1306_draw_string(&ssd, buffer, 5, 36);
    snprintf(buffer, sizeof(buffer), "Usadas: %d", MAX - avaliable_tokens);
    ssd1306_draw_string(&ssd, buffer, 5, 47);

    ssd1306_send_data(&ssd);
}
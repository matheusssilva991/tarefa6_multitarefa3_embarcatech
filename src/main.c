#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "lib/ssd1306/ssd1306.h"
#include "lib/ssd1306/display.h"
#include "lib/led/led.h"
#include "lib/button/button.h"
#include "lib/buzzer/buzzer.h"

// Constantes globais
const uint8_t MAX = 8;
const uint32_t DEBOUNCE_MS = 270; // Tempo de debounce em milissegundos

// Variáveis globais
ssd1306_t ssd;
SemaphoreHandle_t xOutputMutex;
SemaphoreHandle_t xCounterSemphr;
SemaphoreHandle_t xBinBtnSwSemphr;
SemaphoreHandle_t xBinBtnASemphr;
SemaphoreHandle_t xBinBtnBSemphr;
static volatile int64_t last_time_btn_a = 0;
static volatile int64_t last_time_btn_b = 0;
static volatile int64_t last_time_btn_sw = 0;

void vTaskEntrance(void *params);
void vTaskExit(void *params);
void vTaskReset(void *params);
void update_display();
void update_ledRGB();
void gpio_irq_handler(uint gpio, uint32_t events); // Função de interrupção para o botão B

int main()
{
    stdio_init_all();

    // Ativa BOOTSEL via botão SW
    init_display(&ssd);
    init_leds();

    // Inicializa os semáforos
    xOutputMutex = xSemaphoreCreateMutex();
    xCounterSemphr = xSemaphoreCreateCounting(MAX, MAX);
    xBinBtnSwSemphr = xSemaphoreCreateBinary();
    xBinBtnASemphr = xSemaphoreCreateBinary();
    xBinBtnBSemphr = xSemaphoreCreateBinary();

    // Atualiza o display e o LED RGB
    update_display();
    update_ledRGB();

    xTaskCreate(vTaskEntrance, "Task de entrada", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskExit, "Task de saída", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskReset, "Task de reset", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);

    // Inicia o agendador
    vTaskStartScheduler();
    panic_unsupported();
}

// Função de interrupção para os botões
void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint64_t current_time = to_ms_since_boot(get_absolute_time());

    if (gpio == BTN_SW_PIN && current_time - last_time_btn_sw > DEBOUNCE_MS) {
        last_time_btn_sw = current_time;
        xSemaphoreGiveFromISR(xBinBtnSwSemphr, NULL);
    } else if (gpio == BTN_A_PIN && current_time - last_time_btn_a > DEBOUNCE_MS) {
        last_time_btn_a = current_time;
        xSemaphoreGiveFromISR(xBinBtnASemphr, NULL);
    } else if (gpio == BTN_B_PIN && current_time - last_time_btn_b > DEBOUNCE_MS) {
        last_time_btn_b = current_time;
        xSemaphoreGiveFromISR(xBinBtnBSemphr, NULL);
    }
}

// Tarefa de entrada de fichas do RU
// Lógica: quando o botão A é pressionado, pega um token de contagem
// e atualiza o display e o LED RGB
void vTaskEntrance(void *params) {
    init_btn(BTN_A_PIN);
    gpio_set_irq_enabled_with_callback(BTN_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    update_ledRGB();

    while(true) {
        // Aguarda o botão A ser pressionado
        if (xSemaphoreTake(xBinBtnASemphr, portMAX_DELAY) == pdTRUE) {
            // Aguarda o semáforo de contagem
            if (xSemaphoreTake(xCounterSemphr, 0) == pdTRUE) {
                // Pega um token para indicar que uma ficha foi retirada
                if (xSemaphoreTake(xOutputMutex, portMAX_DELAY) == pdTRUE) {
                    update_display();
                    update_ledRGB();
                    xSemaphoreGive(xOutputMutex);
                }
            } else {
                // Lógica para o caso de não haver fichas disponíveis
            }
        }

        vTaskDelay(pdTICKS_TO_MS(150));
    };
}

// Tarefa de saída de fichas do RU
// Lógica: quando o botão B é pressionado, libera um token de contagem
// e atualiza o display e o LED RGB
void vTaskExit(void *params) {
    init_btn(BTN_B_PIN);
    gpio_set_irq_enabled_with_callback(BTN_B_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    while(true) {
        // Aguarda o botão B ser pressionado
        if (xSemaphoreTake(xBinBtnBSemphr, portMAX_DELAY) == pdTRUE) {
            // Libera um token
            if (xSemaphoreGive(xCounterSemphr) == pdTRUE) {
                // Pega um token para indicar que uma ficha foi devolvida
                if (xSemaphoreTake(xOutputMutex, portMAX_DELAY) == pdTRUE) {
                    update_display();
                    update_ledRGB();
                    xSemaphoreGive(xOutputMutex);
                }
            }
        }

        vTaskDelay(pdTICKS_TO_MS(150));
    };
}

// Tarefa de reset do RU
void vTaskReset(void *params) {
    init_btn(BTN_SW_PIN);
    gpio_set_irq_enabled_with_callback(BTN_SW_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    while(true) {
        if (xSemaphoreTake(xBinBtnSwSemphr, portMAX_DELAY) == pdTRUE) {
            // Reseta o contador de fichas
            if (xSemaphoreTake(xOutputMutex, portMAX_DELAY) == pdTRUE) {
                // Libera todos os tokens
                // para indicar que todas as fichas foram devolvidas
                while (uxSemaphoreGetCount(xCounterSemphr) < MAX) {
                    xSemaphoreGive(xCounterSemphr);
                }

                update_display();
                update_ledRGB();
                xSemaphoreGive(xOutputMutex);
            }
        }

        vTaskDelay(pdTICKS_TO_MS(1000));
    };
}

// Atualiza o display com o número de fichas disponíveis
// e o número total de fichas
void update_display() {
    char buffer[40];
    UBaseType_t available_tokens = uxSemaphoreGetCount(xCounterSemphr);

    ssd1306_fill(&ssd, false);
    ssd1306_rect(&ssd, 0, 0, ssd.width, ssd.height, true, false);
    draw_centered_text(&ssd, "Fichas RU", 5);

    snprintf(buffer, sizeof(buffer), "Total: %d", MAX);
    ssd1306_draw_string(&ssd, buffer, 5, 25);
    snprintf(buffer, sizeof(buffer), "Livres: %d",  available_tokens);
    ssd1306_draw_string(&ssd, buffer, 5, 36);
    snprintf(buffer, sizeof(buffer), "Usadas: %d", MAX - available_tokens);
    ssd1306_draw_string(&ssd, buffer, 5, 47);

    ssd1306_send_data(&ssd);
}

// Atualiza o LED RGB de acordo com o número de fichas disponíveis
// Se todas as fichas estão disponíveis, o LED fica azul
// Se não há fichas disponíveis, o LED fica vermelho
// Se há uma ficha disponível, o LED fica amarelo
void update_ledRGB() {
    UBaseType_t available_tokens = uxSemaphoreGetCount(xCounterSemphr);
    if (available_tokens == MAX) {
        set_led_blue();
    } else if (available_tokens == 0) {
        set_led_red();
    } else if (available_tokens == 1) {
        set_led_yellow();
    } else {
        set_led_green();
    }
}

#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

// Definições dos GPIOs
#define RED_PIN     13
#define GREEN_PIN   11
#define BLUE_PIN    12
#define BUZZER_PIN  21
#define BUTTON_A    5
#define BUTTON_B    6

// Handles das tarefas
TaskHandle_t ledTaskHandle = NULL;
TaskHandle_t buzzerTaskHandle = NULL;

// ======== LED RGB =========
void set_led_color(bool R, bool G, bool B) {
    gpio_put(RED_PIN, R);
    gpio_put(GREEN_PIN, G);
    gpio_put(BLUE_PIN, B);
}

void task_led_rgb(void *pvParameters) {
    int cor = 0;

    while (1) {
        switch (cor) {
            case 0:
                set_led_color(1, 0, 0);  // Vermelho
                break;
            case 1:
                set_led_color(0, 0, 1);  // Azul
                break;
            case 2:
                set_led_color(0, 1, 0);  // Verde
                break;
        }

        cor = (cor + 1) % 3;  // Avança para a próxima cor (0 → 1 → 2 → 0 ...)
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ======== BUZZER =========
void pwm_init_buzzer() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (100 * 4096));
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(BUZZER_PIN, 0);
}

void beep(uint duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_gpio_level(BUZZER_PIN, 2048);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    pwm_set_gpio_level(BUZZER_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
}

void task_buzzer(void *pvParameters) {
    while (1) {
        beep(100);  // beep curto
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ======== BOTÕES =========
void task_buttons(void *pvParameters) {
    bool lastA = true, lastB = true;

    while (1) {
        bool currentA = gpio_get(BUTTON_A);
        bool currentB = gpio_get(BUTTON_B);

        if (currentA == false && lastA == true) {  // Pressionado
            if (eTaskGetState(ledTaskHandle) == eSuspended) {
                vTaskResume(ledTaskHandle);
            } else {
                vTaskSuspend(ledTaskHandle);
            }
        }

        if (currentB == false && lastB == true) {
            if (eTaskGetState(buzzerTaskHandle) == eSuspended) {
                vTaskResume(buzzerTaskHandle);
            } else {
                vTaskSuspend(buzzerTaskHandle);
            }
        }

        lastA = currentA;
        lastB = currentB;

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ======== MAIN =========
int main() {
    stdio_init_all();

    // Inicializa GPIOs
    gpio_init(RED_PIN); gpio_set_dir(RED_PIN, GPIO_OUT);
    gpio_init(GREEN_PIN); gpio_set_dir(GREEN_PIN, GPIO_OUT);
    gpio_init(BLUE_PIN); gpio_set_dir(BLUE_PIN, GPIO_OUT);

    gpio_init(BUTTON_A); gpio_set_dir(BUTTON_A, GPIO_IN); gpio_pull_up(BUTTON_A);
    gpio_init(BUTTON_B); gpio_set_dir(BUTTON_B, GPIO_IN); gpio_pull_up(BUTTON_B);

    pwm_init_buzzer();

    // Cria tarefas
    xTaskCreate(task_led_rgb, "LED Task", 256, NULL, 1, &ledTaskHandle);
    xTaskCreate(task_buzzer, "Buzzer Task", 256, NULL, 1, &buzzerTaskHandle);
    xTaskCreate(task_buttons, "Button Task", 256, NULL, 2, NULL);  // Maior prioridade para leitura de botão

    vTaskStartScheduler();

    while (true);  // Nunca chega aqui
}

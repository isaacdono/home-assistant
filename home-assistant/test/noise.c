#include <math.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "libs/neopixel_pio.h" 
#include "libs/ssd1306.h"

// Configuração do Buzzer
#define BUZZER_PIN 21

// Configuração dos Botões
#define BUTTON_A 5
#define BUTTON_B 6

// Configuração da Tela
#define I2C_SDA 14
#define I2C_SCL 15

// Configuração do Microfone
#define MIC_PIN 28  
#define MIC_CHANNEL 2
#define OFFSET 2048      // ADC Pico W vai de 0 a 4095, offset no meio
#define CLAP_THRESHOLD 0.30 
#define NOISE_THRESHOLD 0.70
 
// Configuração da Matriz de LEDs
#define LED_PIN 7       // GPIO do NeoPixel
#define NUM_LEDS 25     // 5x5 Matriz

typedef struct {
    uint8_t buffer[ssd1306_buffer_length];
    struct render_area frame_area;
} Display;

// Estado global
bool listening = false;   // Se a detecção de som está ativa
bool leds_on = false;     // LEDs da matriz
absolute_time_t last_clap_time;
absolute_time_t last_noise_time;
Display display;

void init() {
    stdio_init_all();

    // Inicializa o microfone
    adc_init();
    adc_gpio_init(MIC_PIN);
    adc_select_input(MIC_CHANNEL);  

    // Inicializa os LEDs
    neopixel_init(LED_PIN);

    // Inicializa Buzzer
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f);
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(BUZZER_PIN, 0);

    // Inicializa I2C para OLED
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa Display OLED
    ssd1306_init();
    struct render_area area = {0, ssd1306_width - 1, 0, ssd1306_n_pages - 1};
    display.frame_area = area;
    calculate_render_area_buffer_length(&display.frame_area);
    memset(display.buffer, 0, ssd1306_buffer_length);
    render_on_display(display.buffer, &display.frame_area);

    // Inicializa Botões
    gpio_init(BUTTON_A);
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_pull_up(BUTTON_B);
}

// Alterna os LEDs
void toggle_leds() {
    const uint8_t BLACK[3] = {0, 0, 0};
    const uint8_t WHITE[3] = {25, 25, 25};
    
    leds_on = !leds_on;
    for (int i = 0; i < NUM_LEDS; i++)
    neopixel_set(i, leds_on ? WHITE : BLACK);
    neopixel_write();
    sleep_ms(1000);
}

float get_mean_vu_value(int n) {
    int i; uint16_t adc_value;
    float volume_ratio, sum_volume_level;
    volume_ratio = sum_volume_level = 0;
    
    for (i = 0; i < n; i++) {
        adc_value = adc_read();
        volume_ratio = (float)adc_value / 4095.0;  // Normaliza entre 0 e 1
        sum_volume_level += pow(volume_ratio, 2);
    }
    float mean_vu_level = sum_volume_level / n;
    return mean_vu_level;
}

// Detecta palmas
bool detect_double_clap() {
    float volume_level = get_mean_vu_value(10);
    // printf("Mean VU Level %.2f\n", volume_level);
    if (volume_level > CLAP_THRESHOLD && volume_level < CLAP_THRESHOLD + 0.2) {
        absolute_time_t now = get_absolute_time();
        int64_t time_diff = absolute_time_diff_us(last_clap_time, now) / 1000;
        
        if (time_diff > 200 && time_diff < 1000)
            return true;
        last_clap_time = now;
    }
    return false;
}

// Ativa alarme de intrusão
void activate_alarm() {
    const uint8_t BLACK[3] = {0, 0, 0};
    const uint8_t RED[3] = {25, 0, 0};

    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < NUM_LEDS; j++)
            neopixel_set(j, RED);
        neopixel_write();
        pwm_set_gpio_level(BUZZER_PIN, 5000);

        sleep_ms(200);

        pwm_set_gpio_level(BUZZER_PIN, 0);
        for (int j = 0; j < NUM_LEDS; j++)
            neopixel_set(j, BLACK);
        neopixel_write();

        sleep_ms(200);
    }

}

// Detecta som alto
bool detect_loud_noise() {
    float volume_level = get_mean_vu_value(25);
    // printf("Mean VU Level %.2f\n", volume_level);
    if (volume_level > NOISE_THRESHOLD) {
        // absolute_time_t now = get_absolute_time();
        // int64_t time_diff = absolute_time_diff_us(last_noise_time, now) / 1000;
        
        // if (time_diff > 300 && time_diff < 1000)
            return true;
        // last_noise_time = now;
    }
    return false;
}

// Exibe o menu
void display_menu() {
    memset(display.buffer, 0, ssd1306_buffer_length);
    ssd1306_draw_string(display.buffer, 10, 10, "NOISE DETECTOR");
    ssd1306_draw_string(display.buffer, 5, 30, "A: Listen/Pause");
    ssd1306_draw_string(display.buffer, 5, 40, "B: Exit");
    render_on_display(display.buffer, &display.frame_area);
}

// Loop principal de detecção
void detect_loop() {
    display_menu();
    listening = true;

    while (true) {
        if (listening) {
            if (detect_loud_noise())
                activate_alarm();
            else if (detect_double_clap())
                toggle_leds();
        }
        if (!gpio_get(BUTTON_A)) {
            listening = !listening;  // Alterna entre ouvir e pausar
            sleep_ms(300);
        }
        if (!gpio_get(BUTTON_B)) {
            memset(display.buffer, 0, ssd1306_buffer_length);
            render_on_display(display.buffer, &display.frame_area);
            neopixel_clear();
            break;  // Sai do loop
        }
        sleep_ms(10);
    }
}

// Função principal
int detect_sounds() {
    init();
    detect_loop();
    return 0;
}

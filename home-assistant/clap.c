#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "libs/neopixel_pio.h" 

// Configuração do Buzzer
#define BUZZER_PIN 21

// Configuração do Microfone
#define MIC_PIN 28  
#define OFFSET 32768    
#define CLAP_THRESHOLD 10000  
#define NOISE_THRESHOLD 20000
#define DEBOUNCE_MS 300  
#define DOUBLE_CLAP_TIME 800  

// Configuração da Matriz de LEDs
#define LED_PIN 7       // GPIO do NeoPixel
#define NUM_LEDS 25     // 5x5 Matriz

// Estado dos LEDs
bool leds_on = false;
bool alarm_on = false;
absolute_time_t last_clap_time;
absolute_time_t last_noise_time;

void init() {
    stdio_init_all();

    // Inicializa o microfone
    adc_init();
    adc_gpio_init(MIC_PIN);
    adc_select_input(2);  

    // Inicializa os LEDs
    neopixel_init(LED_PIN);

    // Inicializa Buzzer
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f); // Ajusta divisor de clock
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(BUZZER_PIN, 0); // Desliga o PWM inicialmente
}

// Alterna os LEDs
void toggle_leds() {
    const uint8_t BLACK[3] = {0, 0, 0};
    const uint8_t WHITE[3] = {25, 25, 25};
    
    leds_on = !leds_on;
    if (leds_on) 
        for (int i = 0; i < NUM_LEDS; i++)
            neopixel_set(i, WHITE);
    else
        for (int i = 0; i < NUM_LEDS; i++)
        neopixel_set(i, BLACK);
    neopixel_write();
}

// Detecta palmas
bool detect_clap() {
    uint16_t adc_value = adc_read();
    int volume = (adc_value > OFFSET) ? (adc_value - OFFSET) * 2 : 0;

    if (volume > CLAP_THRESHOLD) {
        absolute_time_t now = get_absolute_time();
        int64_t time_diff = absolute_time_diff_us(last_clap_time, now) / 1000; // Converte para ms

        if (time_diff < DOUBLE_CLAP_TIME)
            toggle_leds();
        last_clap_time = now;  
        sleep_ms(DEBOUNCE_MS);

        return true;
    }
    return false;
}

// Activate intruder alarm
void activate_alarm() {
    const uint8_t BLACK[3] = {0, 0, 0};
    const uint8_t RED[3] = {25, 0, 0};

    alarm_on = true;

    // Flash red LEDs
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < NUM_LEDS; j++) {
            neopixel_set(j, RED); // Red
        }
        neopixel_write();
        sleep_ms(200);
        
        // Turn off LEDs
        for (int j = 0; j < NUM_LEDS; j++) {
            neopixel_set(j, BLACK); // Off
        }
        neopixel_write();
        sleep_ms(200);
    }

    // Sound the buzzer
    for (int i = 0; i < 5; i++) {
        pwm_set_gpio_level(BUZZER_PIN, 5000);
        sleep_ms(200);
        pwm_set_gpio_level(BUZZER_PIN, 0);
        sleep_ms(200);
    }

    alarm_on = false;
}


bool detect_loud_noise() {
    uint16_t adc_value = adc_read();
    int volume = (adc_value > OFFSET) ? (adc_value - OFFSET) * 2 : 0;

    if (volume > NOISE_THRESHOLD) {
        absolute_time_t now = get_absolute_time();
        int64_t time_diff = absolute_time_diff_us(last_noise_time, now) / 1000;

        if (time_diff > DEBOUNCE_MS) {
            activate_alarm();
            last_noise_time = now;
        }

        sleep_ms(DEBOUNCE_MS);
        return true;
    }
    return false;
}

 
int detect_sounds() {
    while(true) {
        if (detect_loud_noise())
            activate_alarm();
        else if (detect_clap())
            toggle_leds();
        sleep_ms(200);
    }
    return 0;
}
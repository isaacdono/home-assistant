#include <stdio.h>
// #include <string.h>
// #include <stdlib.h>
// #include <ctype.h>
#include "pico/stdlib.h"
// #include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "libs/neopixel_pio.h"
#include "libs/ssd1306.h"

const uint BUZZER = 21;
const uint BUZZER_FREQUENCY = 100;
const uint BUTTON_A = 5;  
const uint BUTTON_B = 6;  
const uint LEDS = 7;
const uint I2C_SDA = 14;
const uint I2C_SCL = 15;
const uint X_AXIS = 27;
const uint Y_AXIS = 26; 

// Definição de uma função para inicializar o PWM no pino do buzzer
void pwm_init_buzzer(uint pin) {
    // Configurar o pino como saída de PWM
    gpio_set_function(pin, GPIO_FUNC_PWM);

    // Obter o slice do PWM associado ao pino
    uint slice_num = pwm_gpio_to_slice_num(pin);

    // Configurar o PWM com frequência desejada
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (BUZZER_FREQUENCY * 4096)); // Divisor de clock
    pwm_init(slice_num, &config, true);

    // Iniciar o PWM no nível baixo
    pwm_set_gpio_level(pin, 0);
}


void init() {
    // // Inicializa os tipos stdio padrão presentes ligados ao binário
    // stdio_init_all();   

    // Initialize LED Matrix
    neopixel_init(LEDS);

    // Inicialização do i2c
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicialização do Display
    ssd1306_init();

    // Inicialização do Joystick
    adc_init();
    adc_gpio_init(X_AXIS);
    adc_gpio_init(Y_AXIS);

    // Inicialização dos Botões
    gpio_set_function(BUTTON_A, GPIO_FUNC_SIO); 
    gpio_set_function(BUTTON_B, GPIO_FUNC_SIO); 
    gpio_set_dir(BUTTON_A, GPIO_IN); 
    gpio_set_dir(BUTTON_B, GPIO_IN); 
    gpio_pull_up(BUTTON_A); 
    gpio_pull_up(BUTTON_B); 

    // Inicialização do Buzzer
    pwm_init_buzzer(BUZZER);
}

void light_leds() {
    const uint8_t RED[3] = {25, 0, 0};
    const uint8_t GREEN[3] = {0, 25, 0};
    const uint8_t BLACK[3] = {0, 0, 0};
    const uint8_t colors[5][5][3] = {
    {BLACK, BLACK, GREEN, GREEN, GREEN},
    {BLACK, BLACK, GREEN, BLACK, BLACK},
    {GREEN, GREEN, GREEN, BLACK, BLACK},
    {BLACK, BLACK, BLACK, BLACK, GREEN},
    {RED, BLACK, BLACK, BLACK, BLACK},
    };

    
}








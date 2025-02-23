#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "libs/ssd1306.h"

#include "games/games.h"
#include "music/music.h"
#include "noise/noise.h"

#define BUTTON_A 5
#define BUTTON_B 6

#define I2C_SDA 14
#define I2C_SCL 15

typedef struct {
    uint8_t buffer[ssd1306_buffer_length];
    struct render_area frame_area;
} Display;
Display display;

void show_menu(int option) {
    memset(display.buffer, 0, ssd1306_buffer_length);
    ssd1306_draw_string(display.buffer, 10, 10, "SELECT MODE:");
    ssd1306_draw_string(display.buffer, 5, 30, option == 0 ? "-> Play Game" : "   Play Game");
    ssd1306_draw_string(display.buffer, 5, 40, option == 1 ? "-> Play Music" : "   Play Music");
    ssd1306_draw_string(display.buffer, 5, 50, option == 2 ? "-> Noise Detect" : "   Noise Detect");
    render_on_display(display.buffer, &display.frame_area);
}

int main() {
    stdio_init_all();

    // Inicializa botões
    gpio_init(BUTTON_A);
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_pull_up(BUTTON_B);

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

    int option = 0;
    show_menu(option);

    while (true) {
        if (!gpio_get(BUTTON_A)) {  // Alterna entre opções
            option = (option + 1) % 3;
            show_menu(option);
            sleep_ms(300);
        }
        if (!gpio_get(BUTTON_B)) {  // Confirma seleção
            sleep_ms(300);
            break;
        }
    }

    switch (option) {
        case 0: 
            run_emulator(); 
            break;
        case 1: 
            play_songs(); 
            break;
        case 2: 
            detect_sounds(); 
            break;
    }

    return 0;
}

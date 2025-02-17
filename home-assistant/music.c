#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "libs/ssd1306.h"
#include "libs/neopixel_pio.h"

#define BUZZER1 21
#define BUZZER2 10
#define BUTTON_A 5
#define BUTTON_B 6
#define LED_PIN 7
#define NUM_LEDS 25

bool running = false;
bool next_song = false;

void toggle_pause(uint gpio, uint32_t events) {
    sleep_ms(350);
    running = !running;
}

void skip_song(uint gpio, uint32_t events) {
    sleep_ms(350);
    next_song = true;
}

void play_note(uint buzzer, uint freq, uint duration) {
    if (freq > 0) {
        pwm_set_gpio_level(buzzer, 6000);
        pwm_set_clkdiv_int_frac(pwm_gpio_to_slice_num(buzzer), 4, 0);
        pwm_set_wrap(pwm_gpio_to_slice_num(buzzer), 1250000 / freq);
    }
    sleep_ms(duration);
    pwm_set_gpio_level(buzzer, 0);
    sleep_ms(40);
}

void display_menu(ssd1306_t *oled) {
    ssd1306_clear(oled);
    ssd1306_draw_string(oled, 15, 10, "MUSIC PLAYER");
    ssd1306_draw_string(oled, 10, 30, "A: Play/Pause");
    ssd1306_draw_string(oled, 10, 40, "B: Next Song");
    ssd1306_show(oled);
}

void play_song(const uint (*song)[2], size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (next_song) return;
        while (!running) sleep_ms(200);
        play_note(BUZZER1, song[i][0], song[i][1]);
    }
}

void music_player() {
    stdio_init_all();
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &toggle_pause);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &skip_song);
    
    ssd1306_t oled;
    ssd1306_init(&oled, 128, 64);
    display_menu(&oled);
    
    const uint mario_theme[][2] = {
        {2637, 125}, {2637, 125}, {0, 125}, {2637, 125},
        {0, 125}, {2093, 125}, {2637, 125}, {0, 125},
        {3136, 250}, {0, 125}, {1568, 250}, {0, 250}
    };
    
    const uint imperial_march[][2] = {
        {440, 500}, {440, 500}, {349, 350}, {523, 150},
        {440, 500}, {349, 350}, {523, 150}, {440, 1000}
    };
    
    const uint (*songs[]) = {mario_theme, imperial_march};
    const size_t song_lengths[] = {sizeof(mario_theme)/sizeof(mario_theme[0]), sizeof(imperial_march)/sizeof(imperial_march[0])};
    size_t current_song_index = 0;
    
    while (true) {
        if (running) {
            play_song(songs[current_song_index], song_lengths[current_song_index]);
        }
        if (next_song) {
            current_song_index = (current_song_index + 1) % 2;
            running = true;
            next_song = false;
        }
        sleep_ms(200);
    }
}

int main() {
    music_player();
}

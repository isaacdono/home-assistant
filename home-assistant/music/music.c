#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h"
#include "libs/neopixel_pio.h"
#include "libs/ssd1306.h"

#include "music.h"

// --- Definições de hardware ---
#define BUZZER_1 21
#define BUZZER_2 10
#define BUTTON_A 5
#define BUTTON_B 6
#define LEDS 7
#define I2C_SDA 14
#define I2C_SCL 15

// Estrutura para representar uma nota
typedef struct {
    const char *name;
    uint frequency;
} Note;

// Lista de notas
Note NOTES[] = {
    {"C0", 16}, {"C#0", 17}, {"D0", 18}, {"D#0", 19}, {"E0", 21}, {"F0", 22}, {"F#0", 23}, {"G0", 25}, {"G#0", 26}, {"A0", 28}, {"A#0", 29}, {"B0", 31},
    {"C1", 33}, {"C#1", 35}, {"D1", 37}, {"D#1", 39}, {"E1", 41}, {"F1", 44}, {"F#1", 46}, {"G1", 49}, {"G#1", 52}, {"A1", 55}, {"A#1", 58}, {"B1", 62},
    {"C2", 65}, {"C#2", 69}, {"D2", 73}, {"D#2", 78}, {"E2", 82}, {"F2", 87}, {"F#2", 92}, {"G2", 98}, {"G#2", 104}, {"A2", 110}, {"A#2", 117}, {"B2", 123},
    {"C3", 131}, {"C#3", 139}, {"D3", 147}, {"D#3", 156}, {"E3", 165}, {"F3", 175}, {"F#3", 185}, {"G3", 196}, {"G#3", 208}, {"A3", 220}, {"A#3", 233}, {"B3", 247},
    {"C4", 262}, {"C#4", 277}, {"D4", 294}, {"D#4", 311}, {"E4", 330}, {"F4", 349}, {"F#4", 370}, {"G4", 392}, {"G#4", 415}, {"A4", 440}, {"A#4", 466}, {"B4", 494},
    {"C5", 523}, {"C#5", 554}, {"D5", 587}, {"D#5", 622}, {"E5", 659}, {"F5", 698}, {"F#5", 740}, {"G5", 784}, {"G#5", 831}, {"A5", 880}, {"A#5", 932}, {"B5", 988},
    {"C6", 1047}, {"C#6", 1109}, {"D6", 1175}, {"D#6", 1245}, {"E6", 1319}, {"F6", 1397}, {"F#6", 1480}, {"G6", 1568}, {"G#6", 1661}, {"A6", 1760}, {"A#6", 1865}, {"B6", 1976},
    {"C7", 2093}, {"C#7", 2217}, {"D7", 2349}, {"D#7", 2489}, {"E7", 2637}, {"F7", 2794}, {"F#7", 2960}, {"G7", 3136}, {"G#7", 3322}, {"A7", 3520}, {"A#7", 3729}, {"B7", 3951},
    {"REST", 0}
};

typedef struct {
    const Note *note;
    uint duration;
} Tone;

typedef struct {
    const Tone *sheet;
    uint8_t length;
} Song;

static const Tone mario_theme_sheet[] = {
    {&NOTES[64], 125}, {&NOTES[64], 125}, {&NOTES[88], 125}, {&NOTES[64], 125},
    {&NOTES[88], 125}, {&NOTES[72], 125}, {&NOTES[64], 125}, {&NOTES[88], 125},
    {&NOTES[78], 250}, {&NOTES[88], 125}, {&NOTES[76], 250}, {&NOTES[88], 250},
    {&NOTES[72], 125}, {&NOTES[88], 125}, {&NOTES[64], 125}, {&NOTES[60], 125},
    {&NOTES[67], 125}, {&NOTES[76], 250}, {&NOTES[72], 125}, {&NOTES[67], 125},
    {&NOTES[64], 250}, {&NOTES[88], 250}
};

static const Tone imperial_march_sheet[] = {
    {&NOTES[52], 500}, {&NOTES[52], 500}, {&NOTES[56], 350}, {&NOTES[60], 150},
    {&NOTES[52], 500}, {&NOTES[56], 350}, {&NOTES[60], 150}, {&NOTES[52], 1000},
    {&NOTES[56], 500}, {&NOTES[56], 500}, {&NOTES[60], 350}, {&NOTES[64], 150},
    {&NOTES[56], 500}, {&NOTES[60], 350}, {&NOTES[64], 150}, {&NOTES[56], 1000}
};

const Song mario_theme = {mario_theme_sheet, sizeof(mario_theme_sheet) / sizeof(Tone)};
const Song imperial_march = {imperial_march_sheet, sizeof(imperial_march_sheet) / sizeof(Tone)};
const Song *songs[] = {&mario_theme, &imperial_march};

typedef struct {
    uint8_t buffer[ssd1306_buffer_length];
    struct render_area frame_area;
} Display;

Display display;

// Definição de cores
const uint8_t RED[3] = {25, 0, 0};
const uint8_t MAGENTA[3] = {25, 0, 25};
const uint8_t BLUE[3] = {0, 0, 25};
const uint8_t BLACK[3] = {0, 0, 0};

// Índices da matriz de LEDs
int index_[5][5] = {
    {4, 5, 14, 15, 24},  // Coluna 0
    {3, 6, 13, 16, 23},  // Coluna 1
    {2, 7, 12, 17, 22},  // Coluna 2
    {1, 8, 11, 18, 21},  // Coluna 3
    {0, 9, 10, 19, 20}   // Coluna 4
};

bool running = false;
bool next_song = false;

void button_callback(uint gpio, uint32_t events) {
    if (gpio == BUTTON_A)
        running = !running;
    else if (gpio == BUTTON_B)
        next_song = true;

}

// --- Inicialização ---
void init() {
    stdio_init_all();

    // Inicializa LEDs
    neopixel_init(LEDS);
    
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

    // Interrupções nos botões 
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    
    // Inicializa Buzzer
    gpio_set_function(BUZZER_1, GPIO_FUNC_PWM);
    uint slice_num_1 = pwm_gpio_to_slice_num(BUZZER_1);
    gpio_set_function(BUZZER_2, GPIO_FUNC_PWM);
    uint slice_num_2 = pwm_gpio_to_slice_num(BUZZER_2);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 32.0f); // Ajusta divisor de clock
    pwm_init(slice_num_1, &config, true);
    pwm_init(slice_num_2, &config, true);
    pwm_set_gpio_level(BUZZER_1, 0); // Desliga o PWM inicialmente
    pwm_set_gpio_level(BUZZER_2, 0); // Desliga o PWM inicialmente
}


// --- Limpa LEDs ---
void clear_all() {
    neopixel_clear();
}

// --- Acende LEDs conforme notas
void light_leds(uint frequency, uint duration) {
    int freq = -1;
    
    // Encontrar a frequência da nota
    freq = frequency;
    
    if (freq == -1) return; // Nota inválida
    
    // Determinar a coluna com base na frequência
    int column = -1;
    if (freq < 131) column = 0;
    else if (freq <= 261) column = 1;
    else if (freq < 521) column = 2;
    else if (freq < 1041) column = 3;
    else if (freq < 4187) column = 4;
    
    // Determinar a intensidade com base na duração
    int intensity = -1;
    if (duration < 126) intensity = 0;
    else if (duration < 251) intensity = 1;
    else if (duration < 501) intensity = 2;
    else if (duration < 1001) intensity = 3;
    
    if (column == -1 || intensity == -1) return; // Valores inválidos
    
    // Padrões de cor para cada nível de intensidade
    const uint8_t *color_patterns[4][5] = {
        {BLUE, BLACK, BLACK, BLACK, BLACK},
        {BLUE, MAGENTA, MAGENTA, BLACK, BLACK},
        {BLUE, MAGENTA, MAGENTA, RED, BLACK},
        {BLUE, MAGENTA, MAGENTA, RED, RED}
    };

    // Acender LEDs da coluna correspondente
    for (int i = 0; i < 5; i++) {
        neopixel_set(index_[column][i], color_patterns[intensity][i]);
    }
    neopixel_write();
}

void play_tone(uint frequency, uint duration_ms) {
    if (frequency > 0) {
        uint slice_num_1 = pwm_gpio_to_slice_num(BUZZER_1);
        uint slice_num_2 = pwm_gpio_to_slice_num(BUZZER_2);
        uint32_t clock_freq = clock_get_hz(clk_sys);
        uint32_t top_1 = (clock_freq / 32.0f) / (frequency / 4) - 1;
        uint32_t top_2 = (clock_freq / 32.0f) / (frequency / 8) - 1;
    
        pwm_set_wrap(slice_num_1, top_1 / 2);
        pwm_set_gpio_level(BUZZER_1, top_1 / 8); // reduz duty cycle
        // pwm_set_gpio_level(BUZZER_1, 12000); 
        pwm_set_wrap(slice_num_2, top_2 / 4);
        pwm_set_gpio_level(BUZZER_2, top_2 / 16); // reduz duty cycle
        // pwm_set_gpio_level(BUZZER_1,6000); 
    }

    sleep_ms(duration_ms);
    pwm_set_gpio_level(BUZZER_1, 0); // Desliga o som após a duração
    pwm_set_gpio_level(BUZZER_2, 0); // Desliga o som após a duração
    sleep_ms(50); // Pausa entre notas
}

void play_song(Song song) {
    for (int i = 0; i < song.length; i++) {
        clear_all();
        if (next_song) 
            return;
        while (!running) {
            if (next_song) 
                return;
            sleep_ms(200);
        }
        play_tone(song.sheet[i].note->frequency, song.sheet[i].duration);
        light_leds(song.sheet[i].note->frequency, song.sheet[i].duration);
    }
}

void pause_song() {
    pwm_set_gpio_level(BUZZER_1, 0); // Desliga o som após a duração
    pwm_set_gpio_level(BUZZER_2, 0); // Desliga o som após a duração
}

void display_menu() {
    memset(display.buffer, 0, ssd1306_buffer_length);
    ssd1306_draw_string(display.buffer, 25, 10, "MUSIC PLAYER");
    ssd1306_draw_string(display.buffer, 10, 30, "A: Play/Pause");
    ssd1306_draw_string(display.buffer, 10, 40, "B: Next Song");
    render_on_display(display.buffer, &display.frame_area);
}

void music_player() {
    int current_song_index = 0;
    int n = sizeof(songs) / sizeof(Song*);
    display_menu();

    while (true) {
        if (running)
            // printf("Running!\n");
            play_song(*songs[current_song_index]);

        if (next_song) {
            current_song_index = (current_song_index + 1) % n;
            running = true;
            next_song = false;
        }

        pause_song(); // Garante que o som pare se estiver pausado
        sleep_ms(200); 
    }
}


int play_songs() {
    init();
    music_player();
    return 0;
}
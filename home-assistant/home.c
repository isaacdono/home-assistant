#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "libs/neopixel_pio.h"
#include "libs/ssd1306.h"

// Configuração doS Buzzers
#define BUZZER_1 21
#define BUZZER_2 10

// Configuração dos Botões
#define BUTTON_A 5
#define BUTTON_B 6

// Configuração da Tela
#define I2C_SDA 14
#define I2C_SCL 15

// Configuração do Joystick
#define X_AXIS 27
#define Y_AXIS 26

// Configuração da Matriz de LEDs
#define LEDS_MATRIX 7       // GPIO do NeoPixel
#define NUM_LEDS 25     // 5x5 Matriz

typedef struct {
    uint8_t buffer[ssd1306_buffer_length];
    struct render_area frame_area;
} Display;

Display display;

// ----------------------------------------------
// ----------------------------------------------
// ------------- JOGO DA COBRINHA ---------------
// ----------------------------------------------
// ----------------------------------------------

// --- Estruturas ---
typedef struct {
    int x, y;
} Position;

typedef struct {
    int length;
    Position body[50];
    Position dir;
} Snake;

// --- Variáveis Globais ---
Snake snake;
Position food;
bool running;


void init_game() {
    // // Inicializa LEDs
    // neopixel_init(LEDS_MATRIX);
    
    // Inicializa Joystick
    adc_init();
    adc_gpio_init(X_AXIS);
    adc_gpio_init(Y_AXIS);
    
    // Inicializa Buzzer
    gpio_set_function(BUZZER_1, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_1);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f); // Ajusta divisor de clock
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(BUZZER_1, 0); // Desliga o PWM inicialmente
    
}


void light_snake_leds() {
    int i, j;
    const uint8_t RED[3] = {25, 0, 0};
    const uint8_t GREEN[3] = {0, 25, 0};
    const uint8_t BLACK[3] = {0, 0, 0};
    const uint8_t *colors[5][5] = {
        {BLACK, BLACK, GREEN, GREEN, GREEN},
        {BLACK, BLACK, GREEN, BLACK, BLACK},
        {GREEN, GREEN, GREEN, BLACK, BLACK},
        {BLACK, BLACK, BLACK, BLACK, GREEN},
        {RED, BLACK, BLACK, BLACK, BLACK},
    };
    
    for (i = 0; i < 5; i++)
        for (j = 0; j < 5; j++)
            neopixel_set(i*5 + j, colors[i][j]);
    
    neopixel_write();
}

// --- Limpa LEDs ---
void clear_all() {
    neopixel_clear();
}

// --- Gera nova comida ---
void generate_food() {
    bool valid;
    int i;
    
    do {
        food.x = (rand() % 24 + 1) * 5;
        food.y = (rand() % 12 + 1) * 5;
        valid = true;
        
        // Verifica se a posição gerada coincide com alguma parte da cobra
        for (i = 0; i < snake.length; i++) {
            if (food.x == snake.body[i].x && food.y == snake.body[i].y) {
                valid = false;
                break;
            }
        }
    } while (!valid);
}


// --- Toca um som no buzzer ---
void play_snake_tone(uint frequency, uint duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_1);
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t top = clock_freq / frequency - 1;
    
    pwm_set_wrap(slice_num, top);
    pwm_set_gpio_level(BUZZER_1, top / 2); // 50% de duty cycle
    
    sleep_ms(duration_ms);
    
    pwm_set_gpio_level(BUZZER_1, 0); // Desliga o som após a duração
    // sleep_ms(50); // Pausa entre notas
}

// --- Leitura do joystick ---
void read_joystick() {
    const int threshold = 2000; // Valor limite para definir movimento
    
    adc_select_input(0);
    int y_val = adc_read();
    adc_select_input(1);
    int x_val = adc_read();
    
    // Normaliza para um range de -2048 a 2047
    int x_centered = x_val - 2048;
    int y_centered = y_val - 2048;
    
    // Verifica se o joystick está sendo movido o suficiente
    if (y_centered > threshold && snake.dir.y == 0) {
        snake.dir.x = 0;
        snake.dir.y = -5;
    } else if (y_centered < -threshold && snake.dir.y == 0) {
        snake.dir.x = 0;
        snake.dir.y = 5;
    } else if (x_centered > threshold && snake.dir.x == 0) {
        snake.dir.x = 5;
        snake.dir.y = 0;
    } else if (x_centered < -threshold && snake.dir.x == 0) {
        snake.dir.x = -5;
        snake.dir.y = 0;
    }
}

void move_snake() {
    Position new_head = {snake.body[0].x + snake.dir.x, snake.body[0].y + snake.dir.y};
    
    if (new_head.x < 0 || new_head.x >= 128 || new_head.y < 0 || new_head.y >= 64) {
        running = false;
        return;
    }
    
    for (int i = snake.length - 1; i > 0; i--)
    snake.body[i] = snake.body[i - 1];
    snake.body[0] = new_head;
    
    if (new_head.x == food.x && new_head.y == food.y) {
        snake.length++;
        play_snake_tone(660, 150);
        generate_food();
    }
}

// --- Desenha no OLED ---
void draw_game() {
    memset(display.buffer, 0, ssd1306_buffer_length);
    
    for (int i = 0; i < snake.length; i++)
    ssd1306_set_pixel(display.buffer, snake.body[i].x, snake.body[i].y, 1);
    
    ssd1306_set_pixel(display.buffer, food.x, food.y, 1);
    render_on_display(display.buffer, &display.frame_area);
}

// --- Tela inicial ---
void start_game() {
    memset(display.buffer, 0, ssd1306_buffer_length);
    ssd1306_draw_string(display.buffer, 25, 10, "SNAKE GAME");
    ssd1306_draw_string(display.buffer, 10, 30, "Press A: Start");
    ssd1306_draw_string(display.buffer, 10, 40, "Press B: Exit");
    render_on_display(display.buffer, &display.frame_area);
    
    while (true) {
        if (!gpio_get(BUTTON_A)) {
            // Inicialização da Snake
            running = true;
            snake.length = 3;
            snake.body[0] = (Position){30, 30};
            snake.body[1] = (Position){25, 30};
            snake.body[2] = (Position){20, 30};
            snake.dir.x = 5; // Movendo para a direita
            snake.dir.y = 0;
            generate_food();
            return;
        } else if (!gpio_get(BUTTON_B)) {
            running = false;
            return;
        }
        sleep_ms(100);
    }
}


// --- Tela de Game Over ---
void game_over() {
    memset(display.buffer, 0, ssd1306_buffer_length);
    ssd1306_draw_string(display.buffer, 25, 20, "GAME OVER!");
    
    char score_text[20];
    sprintf(score_text, "Score: %d", snake.length - 3);
    ssd1306_draw_string(display.buffer, 25, 40, score_text);
    
    render_on_display(display.buffer, &display.frame_area);
    play_snake_tone(300, 300);
    sleep_ms(2500);
    
}

// --- Loop do jogo ---
void game_loop() {
    light_snake_leds();
    
    while (true) {
        start_game(); // Tela inicial
        
        if (!running) { // Jogador saiu do jogo
            memset(display.buffer, 0, ssd1306_buffer_length);
            render_on_display(display.buffer, &display.frame_area);
            clear_all();
            return;
        }
        
        while (running) {
            read_joystick();
            move_snake();
            draw_game();
            sleep_ms(250); // Velocidade do jogo
        }
        
        game_over();
    }
}


// --- Programa principal ---
int run_emulator() {
    init_game();
    game_loop();
    return 0;
}

// ----------------------------------------------
// ----------------------------------------------
// ------------- PLAYER DE MÚSICA ---------------
// ----------------------------------------------
// ----------------------------------------------

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

// static const Tone mario_theme_sheet[] = {
//     {&NOTES[64], 125}, {&NOTES[64], 125}, {&NOTES[88], 125}, {&NOTES[64], 125},
//     {&NOTES[88], 125}, {&NOTES[72], 125}, {&NOTES[64], 125}, {&NOTES[88], 125},
//     {&NOTES[78], 250}, {&NOTES[88], 125}, {&NOTES[76], 250}, {&NOTES[88], 250},
//     {&NOTES[72], 125}, {&NOTES[88], 125}, {&NOTES[64], 125}, {&NOTES[60], 125},
//     {&NOTES[67], 125}, {&NOTES[76], 250}, {&NOTES[72], 125}, {&NOTES[67], 125},
//     {&NOTES[64], 250}, {&NOTES[88], 250}
// };

static const Tone imperial_march_sheet[] = {
    {&NOTES[52], 500}, {&NOTES[52], 500}, {&NOTES[56], 350}, {&NOTES[60], 150},
    {&NOTES[52], 500}, {&NOTES[56], 350}, {&NOTES[60], 150}, {&NOTES[52], 1000},
    {&NOTES[56], 500}, {&NOTES[56], 500}, {&NOTES[60], 350}, {&NOTES[64], 150},
    {&NOTES[56], 500}, {&NOTES[60], 350}, {&NOTES[64], 150}, {&NOTES[56], 1000}
};

// const Song mario_theme = {mario_theme_sheet, sizeof(mario_theme_sheet) / sizeof(Tone)};
const Song imperial_march = {imperial_march_sheet, sizeof(imperial_march_sheet) / sizeof(Tone)};
const Song *songs[] = {&imperial_march};

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
// bool next_song = false;
bool back = false;

void button_callback(uint gpio, uint32_t events) {
    if (gpio == BUTTON_A)
    running = !running;
    else if (gpio == BUTTON_B)
    back = true;
    
}

void init_player() {
    // // Inicializa LEDs
    // neopixel_init(LEDS_MATRIX);
    
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


// --- Acende LEDs conforme notas
void light_music_leds(uint frequency, uint duration) {
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

void play_music_tone(uint frequency, uint duration_ms) {
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


void pause_song() {
    pwm_set_gpio_level(BUZZER_1, 0); // Desliga o som após a duração
    pwm_set_gpio_level(BUZZER_2, 0); // Desliga o som após a duração
}

void play_song(Song song) {
    for (int i = 0; i < song.length; i++) {
        clear_all();
        if (back) 
            return;
        while (!running) {
            // pause_song();
            sleep_ms(200);
            if (back) 
                return;
        }
        play_music_tone(song.sheet[i].note->frequency, song.sheet[i].duration);
        light_music_leds(song.sheet[i].note->frequency, song.sheet[i].duration);
    }
}

void display_music_menu() {
    memset(display.buffer, 0, ssd1306_buffer_length);
    ssd1306_draw_string(display.buffer, 25, 10, "MUSIC PLAYER");
    ssd1306_draw_string(display.buffer, 10, 30, "A: Play/Pause");
    ssd1306_draw_string(display.buffer, 10, 40, "B: Exit");
    render_on_display(display.buffer, &display.frame_area);
}

void music_player() {
    int current_song_index = 0;
    int n = sizeof(songs) / sizeof(Song*);
    display_music_menu();
    
    while (true) {
        if (running) {
            // printf("Running!\n");
            sleep_ms(50);
            play_song(*songs[current_song_index]);
        }
        
        if (back) {
            sleep_ms(300);
            running = false;
            back = false;
            memset(display.buffer, 0, ssd1306_buffer_length);
            render_on_display(display.buffer, &display.frame_area);
            clear_all();
            return;
        }
        
        pause_song(); // Garante que o som pare se estiver pausado
        sleep_ms(200); 
    }
}

int play_songs() {
    init_player();
    music_player();
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, false, &button_callback);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, false, &button_callback);

    return 0;
}


// ----------------------------------------------
// ----------------------------------------------
// ------------- DETECTOR DE SONS ---------------
// ----------------------------------------------
// ----------------------------------------------

// Configuração do Microfone
#define MIC_PIN 28  
#define MIC_CHANNEL 2
#define OFFSET 2048      // ADC Pico W vai de 0 a 4095, offset no meio
#define CLAP_THRESHOLD 0.30 
#define NOISE_THRESHOLD 0.60

// Estado global
bool listening = false;   // Se a detecção de som está ativa
bool leds_on = false;     // LEDs da matriz
absolute_time_t last_clap_time;
absolute_time_t last_noise_time;

void init_detector() {
    // Inicializa o microfone
    adc_init();
    adc_gpio_init(MIC_PIN);
    adc_select_input(MIC_CHANNEL);  
    
    // // Inicializa os LEDs
    // neopixel_init(LEDS_MATRIX);
    
    // Inicializa Buzzer
    gpio_set_function(BUZZER_1, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_1);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f);
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(BUZZER_1, 0);
    
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
    sum_volume_level = 0;
    
    for (i = 0; i < n; i++) {
        adc_value = adc_read();
        volume_ratio = (float)adc_value / 4095.0;  // Normaliza entre 0 e 1
        sum_volume_level += pow(volume_ratio, 2);
        sleep_ms(1);
    }
    float mean_vu_level = sum_volume_level / (float)n;
    return mean_vu_level;
}

// Detecta palmas
bool detect_double_clap() {
    float volume_level = get_mean_vu_value(5);
    printf("DC VU Level %.2f\n", volume_level);
    if (volume_level > CLAP_THRESHOLD && volume_level < (CLAP_THRESHOLD + 0.20)) {
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
        pwm_set_gpio_level(BUZZER_1, 5000);
        
        sleep_ms(200);
        
        pwm_set_gpio_level(BUZZER_1, 0);
        for (int j = 0; j < NUM_LEDS; j++)
        neopixel_set(j, BLACK);
        neopixel_write();
        
        sleep_ms(200);
    }
    
}

// Detecta som alto
bool detect_loud_noise() {
    float volume_level = get_mean_vu_value(30);
    printf("Noise VU Level %.2f\n", volume_level);
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
void display_noise_menu() {
    memset(display.buffer, 0, ssd1306_buffer_length);
    ssd1306_draw_string(display.buffer, 10, 10, "NOISE DETECTOR");
    ssd1306_draw_string(display.buffer, 5, 30, "A: Listen/Stop");
    ssd1306_draw_string(display.buffer, 5, 40, "B: Exit");
    render_on_display(display.buffer, &display.frame_area);
}

// Loop principal de detecção

void detect_loop() {
    display_noise_menu();
    listening = true;
    while (true) {
            if (listening) {
                if (detect_loud_noise())
                activate_alarm();
                else if (detect_double_clap())
                toggle_leds();
            }
            if (!gpio_get(BUTTON_A)) {
                sleep_ms(300);
                listening = !listening;  // Alterna entre ouvir e pausar
            }
            else if (!gpio_get(BUTTON_B)) {
                sleep_ms(300);
                memset(display.buffer, 0, ssd1306_buffer_length);
                render_on_display(display.buffer, &display.frame_area);
                clear_all();
                break;  // Sai do loop
            }
            sleep_ms(10);
        }
}

// Função principal
int detect_sounds() {
    init_detector();
    detect_loop();
    return 0;
}

// ----------------------------------------------
// ----------------------------------------------
// -------------- MENU PRINCIPAL ----------------
// ----------------------------------------------
// ----------------------------------------------

void init() {
    stdio_init_all();
    
    // Inicializa LEDs
    neopixel_init(LEDS_MATRIX);
    
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

void light_home_leds() {
    int i, j;

    const uint8_t WHITE[3] = {15, 15, 15};
    const uint8_t BLACK[3] = {0, 0, 0};

    const uint8_t *colors[5][5] = {
        {BLACK, WHITE, WHITE, WHITE, BLACK},
        {BLACK, WHITE, BLACK, WHITE, BLACK},
        {WHITE, WHITE, WHITE, WHITE, WHITE},
        {BLACK, WHITE, WHITE, WHITE, BLACK},
        {BLACK, BLACK, WHITE, BLACK, BLACK},
    };
    
    for (i = 0; i < 5; i++)
        for (j = 0; j < 5; j++)
            neopixel_set(i*5 + j, colors[i][j]);
    
    neopixel_write();
}

void show_menu(int option) {
    printf("Menu is on!\n");
    memset(display.buffer, 0, ssd1306_buffer_length);
    ssd1306_draw_string(display.buffer, 20, 10, "SELECT MODE:");
    ssd1306_draw_string(display.buffer, 0, 25, option == 0 ? "x Snake Game" : "  Snake Game");
    ssd1306_draw_string(display.buffer, 0, 35, option == 1 ? "x Music Player" : "  Music Player");
    ssd1306_draw_string(display.buffer, 0, 45, option == 2 ? "x Noise Detect" : "  Noise Detect");
    render_on_display(display.buffer, &display.frame_area);
}

int main() {
    init();
    
    int option = 0;
    
    show_menu(option);
    light_home_leds();
    while (true) {
        if (!gpio_get(BUTTON_A)) {  // Alterna entre opções
            sleep_ms(300);
            option = (option + 1) % 3;
            show_menu(option);
        }
        else if (!gpio_get(BUTTON_B)) {  // Confirma seleção
            clear_all();
            sleep_ms(300);
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
            show_menu(option);
            light_home_leds();
        }
        sleep_ms(200);
    }

    return 0;
}

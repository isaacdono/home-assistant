#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "libs/neopixel_pio.h"
#include "libs/ssd1306.h"

// --- Definições de hardware ---
#define BUZZER 21
#define BUTTON_A 5
#define BUTTON_B 6
#define LEDS 7
#define I2C_SDA 14
#define I2C_SCL 15
#define X_AXIS 27
#define Y_AXIS 26

// --- Definições do jogo ---
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define MAX_SNAKE_LENGTH 50

// --- Estruturas ---
typedef struct {
    int x, y;
} Position;

typedef struct {
    int length;
    Position body[MAX_SNAKE_LENGTH];
    Position dir;
} Snake;

typedef struct {
    uint8_t buffer[ssd1306_buffer_length];
    struct render_area frame_area;
} Display;

// --- Variáveis Globais ---
Snake snake;
Position food;
Display display;
bool running;

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

    // Inicializa Joystick
    adc_init();
    adc_gpio_init(X_AXIS);
    adc_gpio_init(Y_AXIS);

    // Inicializa Botões
    gpio_init(BUTTON_A);
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_pull_up(BUTTON_B);
    
    // Inicializa Buzzer
    gpio_set_function(BUZZER, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f); // Ajusta divisor de clock
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(BUZZER, 0); // Desliga o PWM inicialmente

}

// --- Acende LEDs da cobra e da comida ---
void light_leds() {
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
void play_tone(uint frequency, uint duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER);
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t top = clock_freq / frequency - 1;

    pwm_set_wrap(slice_num, top);
    pwm_set_gpio_level(BUZZER, top / 2); // 50% de duty cycle

    sleep_ms(duration_ms);

    pwm_set_gpio_level(BUZZER, 0); // Desliga o som após a duração
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

// --- Move a cobra ---
void move_snake() {
    Position new_head = {snake.body[0].x + snake.dir.x, snake.body[0].y + snake.dir.y};

    if (new_head.x < 0 || new_head.x >= OLED_WIDTH || new_head.y < 0 || new_head.y >= OLED_HEIGHT) {
        running = false;
        return;
    }

    for (int i = snake.length - 1; i > 0; i--)
        snake.body[i] = snake.body[i - 1];
    snake.body[0] = new_head;

    if (new_head.x == food.x && new_head.y == food.y) {
        snake.length++;
        play_tone(660, 150);
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
    play_tone(300, 300);
    sleep_ms(2500);

}


// --- Loop do jogo ---
void game_loop() {
    light_leds();

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
    init();
    game_loop();
    return 0;
}

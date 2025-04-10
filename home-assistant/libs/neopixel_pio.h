#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

// Biblioteca gerada pelo arquivo .pio durante compilação.
#include "ws2818b.pio.h"

// Definição do número de LEDs e pino.
#define LED_COUNT 25
#define LED_PIN 7

// Definição de pixel GRB
struct pixel_t
{
    uint8_t G, R, B; // Três valores de 8-bits compõem um pixel.
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; // Mudança de nome de "struct pixel_t" para "npLED_t" por clareza.

// Declaração do buffer de pixels que formam a matriz.
npLED_t leds[LED_COUNT];

// Variáveis para uso da máquina PIO.
PIO np_pio;
uint sm;

/**
 * Inicializa a máquina PIO para controle da matriz de LEDs.
 */
void neopixel_init(uint pin)
{

    // Cria programa PIO.
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;

    // Toma posse de uma máquina PIO.
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0)
    {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true); // Se nenhuma máquina estiver livre, panic!
    }

    // Inicia programa na máquina PIO obtida.
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

    // Limpa buffer de pixels.
    for (uint i = 0; i < LED_COUNT; ++i)
    {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}


/**
 * Ajusta a intensidade dos LEDs, obtido no Discord
 */
uint8_t reverse_byte(uint8_t b) {
    uint8_t reversed = 0; // Inicializa o byte a ser revertido
  
    // Faz a reversão bit a bit seguindo a lógica: 
    // 1. Captura o i-ésimo bit do byte original ((b >> i) & 0x1)
    // 2. Envia-o para a variável reversed com o operador |
    // 3. Arrasta o bit transferido para a esquerda (reversed << 1)
    for (uint8_t i = 0; i < 8; i++) {
      reversed = reversed << 1; // 
      reversed = (reversed | ((b >> i) & 0x1)); 
    }
  
    return reversed;
  }

/**
 * Atribui uma cor RGB a um LED.
 */
void neopixel_set(const uint index, const uint8_t color[3]) {

    leds[index].R = reverse_byte(color[0]);
    leds[index].G = reverse_byte(color[1]);
    leds[index].B = reverse_byte(color[2]);
}


/**
 * Escreve os dados do buffer nos LEDs.
 */
void neopixel_write()
{
    // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
    for (uint i = 0; i < LED_COUNT; ++i)
    {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100); // Espera 100us, sinal de RESET do datasheet.
}

/**
 * Limpa o buffer de pixels.
 */
void neopixel_clear() {
    for (uint i = 0; i < LED_COUNT; i++) {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
    neopixel_write();
}
// int main() {

//   // Inicializa entradas e saídas.
//   stdio_init_all();

//   // Inicializa matriz de LEDs NeoPixel.
//   npInit(LED_PIN);
//   npClear();

//   // Aqui, você desenha nos LEDs.

//   npWrite(); // Escreve os dados nos LEDs.

//   // Não faz mais nada. Loop infinito.
//   while (true) {
//     sleep_ms(1000);
//   }
// }

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

// Constants
#define MIC_PIN 28       // ADC input pin
#define OFFSET 32768     // Baseline (1.65V)
#define THRESHOLD 10000  // Clap detection threshold (adjust as needed)
#define DEBOUNCE_MS 300  // Delay to prevent multiple detections

void init_microphone() {
    adc_init();
    adc_gpio_init(MIC_PIN);
    adc_select_input(2);  // ADC2 corresponds to GPIO28
}

bool detect_clap() {
    uint16_t adc_value = adc_read();
    int volume = (adc_value > OFFSET) ? (adc_value - OFFSET) * 2 : 0; // Ensure non-negative

    if (volume > THRESHOLD) {
        printf("Clap detected!\n");
        sleep_ms(DEBOUNCE_MS);
        return true;
    }
    return false;
}

int main() {
    stdio_init_all();
    init_microphone();
    
    printf("Clap detection started...\n");

    while (1) {
        detect_clap();
        sleep_ms(50);  // Sampling delay
    }

    return 0;
}

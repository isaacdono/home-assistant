import time, math
from machine import Pin, SoftI2C, ADC
from ssd1306 import SSD1306_I2C

# Initialize Microphone
adc = ADC(Pin(28))
OFFSET = int(1.65 / 3.3 * 65536)  # ADC baseline (1.65V)

# Initialize Screen
i2c = SoftI2C(scl=Pin(15), sda=Pin(14))
oled = SSD1306_I2C(128, 64, i2c)

def vu_meter(adc_value):
    volume = max(0, (adc_value - OFFSET) * 2)  # Ensure non-negative
    volume_ratio = volume / 65535  # Normalize
    return math.pow(volume_ratio, 0.5)  # Adjust sensitivity

def listen():
    while True:
        adc_value = adc.read_u16()
        volume_level = vu_meter(adc_value)

        if (volume_level > 0.20):
            print("Clap detected!")
            time.sleep_ms(300)

        # print(f"Volume: {volume_level:.2f}")  # Print volume level
        time.sleep_ms(50)  # Faster sampling for better response

def display_menu():
    oled.fill(0)
    oled.text("VOICE ASSISTANT", 5, 10)
    # oled.text("A: Listen", 10, 30)
    # oled.text("B: Pause", 10, 40)
    oled.show()

display_menu()
listen()
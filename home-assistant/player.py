import time
from machine import Pin, PWM, SoftI2C
from ssd1306 import SSD1306_I2C
import neopixel

RED = (25, 0, 0)
GREEN = (0, 25, 0)
BLUE = (0, 0, 50)
YELLOW = (30, 30, 0)
MAGENTA = (30, 0, 30)
CYAN = (0, 30, 30)
WHITE = (25, 25, 25)
BLACK = (0, 0, 0)
NUM_LEDS = 25

# Initialize LED Matrix
np = neopixel.NeoPixel(Pin(7), NUM_LEDS)

# Initialize Screen
i2c = SoftI2C(scl=Pin(15), sda=Pin(14))
oled = SSD1306_I2C(128, 64, i2c)

# Initialize Buzzers
buzzer1 = PWM(Pin(21))  # Main melody
buzzer2 = PWM(Pin(10))  # Harmonic support

# Initialize Buttons
button_a = Pin(5, Pin.IN, Pin.PULL_UP)
button_b = Pin(6, Pin.IN, Pin.PULL_UP)

# Define notes (frequencies in Hz)
NOTES = {
    "C0": 16, "C#0": 17, "D0": 18, "D#0": 19, "E0": 21, "F0": 22, "F#0": 23, "G0": 25, "G#0": 26, "A0": 28, "A#0": 29, "B0": 31,
    "C1": 33, "C#1": 35, "D1": 37, "D#1": 39, "E1": 41, "F1": 44, "F#1": 46, "G1": 49, "G#1": 52, "A1": 55, "A#1": 58, "B1": 62,
    "C2": 65, "C#2": 69, "D2": 73, "D#2": 78, "E2": 82, "F2": 87, "F#2": 92, "G2": 98, "G#2": 104, "A2": 110, "A#2": 117, "B2": 123,
    "C3": 131, "C#3": 139, "D3": 147, "D#3": 156, "E3": 165, "F3": 175, "F#3": 185, "G3": 196, "G#3": 208, "A3": 220, "A#3": 233, "B3": 247,
    "C4": 262, "C#4": 277, "D4": 294, "D#4": 311, "E4": 330, "F4": 349, "F#4": 370, "G4": 392, "G#4": 415, "A4": 440, "A#4": 466, "B4": 494,
    "C5": 523, "C#5": 554, "D5": 587, "D#5": 622, "E5": 659, "F5": 698, "F#5": 740, "G5": 784, "G#5": 831, "A5": 880, "A#5": 932, "B5": 988,
    "C6": 1047, "C#6": 1109, "D6": 1175, "D#6": 1245, "E6": 1319, "F6": 1397, "F#6": 1480, "G6": 1568, "G#6": 1661, "A6": 1760, "A#6": 1865, "B6": 1976,
    "C7": 2093, "C#7": 2217, "D7": 2349, "D#7": 2489, "E7": 2637, "F7": 2794, "F#7": 2960, "G7": 3136, "G#7": 3322, "A7": 3520, "A#7": 3729, "B7": 3951,
    "REST": 0
}

# Mario Theme 
mario_theme = [
    ("E7", 125), ("E7", 125), ("REST", 125), ("E7", 125),
    ("REST", 125), ("C7", 125), ("E7", 125), ("REST", 125),
    ("G7", 250), ("REST", 125), ("G6", 250), ("REST", 250),

    ("C7", 125), ("REST", 125), ("G6", 125), ("REST", 125),
    ("E6", 125), ("REST", 125), ("A6", 125), ("REST", 125),
    ("B6", 125), ("REST", 125), ("A#6", 125), ("A6", 125),

    ("G6", 125), ("E7", 125), ("G7", 125), ("A7", 250),
    ("REST", 125), ("F7", 125), ("G7", 125), ("REST", 125),
    ("E7", 125), ("REST", 125), ("C7", 125), ("D7", 125),
    ("B6", 125), ("REST", 250),

    ("C7", 125), ("REST", 125), ("G6", 125), ("REST", 125),
    ("E6", 125), ("REST", 125), ("A6", 125), ("REST", 125),
    ("B6", 125), ("REST", 125), ("A#6", 125), ("A6", 125),

    ("G6", 125), ("E7", 125), ("G7", 125), ("A7", 250),
    ("REST", 125), ("F7", 125), ("G7", 125), ("REST", 125),
    ("E7", 125), ("REST", 125), ("C7", 125), ("D7", 125),
    ("B6", 125), ("REST", 250)
]

# Imperial March Theme
imperial_march = [
    ("A4", 500), ("A4", 500), ("F4", 350), ("C5", 150),
    ("A4", 500), ("F4", 350), ("C5", 150), ("A4", 1000),
    ("E5", 500), ("E5", 500), ("E5", 500), ("F5", 350), ("C5", 150),
    ("G4", 500), ("F4", 350), ("C5", 150), ("A4", 1000),

    ("A5", 500), ("A4", 350), ("A4", 150), ("A5", 500), ("G#5", 250), ("G5", 250),
    ("F#5", 125), ("F5", 125), ("F#5", 250), ("REST", 250), ("A#4", 250), ("D#5", 500),
    ("D5", 250), ("C#5", 250), ("C5", 125), ("B4", 125), ("C5", 250), ("REST", 250),

    ("F4", 125), ("G4", 500), ("F4", 375), ("A4", 125), ("C5", 500),
    ("A4", 375), ("C5", 125), ("E5", 1000),

    ("A5", 500), ("A4", 350), ("A4", 150), ("A5", 500), ("G#5", 250), ("G5", 250),
    ("F#5", 125), ("F5", 125), ("F#5", 250), ("REST", 250), ("A#4", 250), ("D#5", 500),
    ("D5", 250), ("C#5", 250), ("C5", 125), ("B4", 125), ("C5", 250), ("REST", 250),

    ("F4", 125), ("G4", 500), ("F4", 375), ("C5", 125), ("A4", 500),
    ("F4", 375), ("C5", 125), ("A4", 1000)
]

# List of available songs
songs = [mario_theme, imperial_march]

# Global state variables
running = False
next_song = False

# Interrupt Handlers
def toggle_pause(pin):
    time.sleep_ms(350)
    global running
    running = not running  # Toggle play/pause

def skip_song(pin):
    time.sleep_ms(350)
    global next_song
    next_song = True  # Signal to skip song

# Attach interrupts to buttons
button_a.irq(trigger=Pin.IRQ_FALLING, handler=toggle_pause)
button_b.irq(trigger=Pin.IRQ_FALLING, handler=skip_song)

# Function to play a note with improved sound
def play_note(note, duration):
    freq = NOTES[note]

    if freq > 0:
        buzzer1.freq(freq)
        buzzer2.freq(freq // 2)  # Adds a harmonic undertone
        buzzer1.duty_u16(12000)  # Softer duty cycle
        buzzer2.duty_u16(6000)

    time.sleep_ms(duration)

    # Brief silence for better staccato effect
    buzzer1.duty_u16(0)
    buzzer2.duty_u16(0)
    time.sleep_ms(40)

# Play song with interrupt-based pausing and skipping
def play_song(song):
    global running, next_song

    for note, duration in song:
        clear_all()
        if next_song:  # Skip song immediately
            return

        while not running:  # Pause handling
            if next_song:
                return
            time.sleep(0.2)

        light_leds(note, duration)
        play_note(note, duration)

# Stop sound
def pause_song():
    buzzer1.duty_u16(0)
    buzzer2.duty_u16(0)

# Display song menu
def display_menu():
    oled.fill(0)
    oled.text("MUSIC PLAYER", 15, 10)
    oled.text("A: Play/Pause", 10, 30)
    oled.text("B: Next Song", 10, 40)
    oled.show()

# Equalizer on LED matrix
def light_leds(note, duration):
    freq = NOTES[note]

    index = [
        [4, 5, 14, 15, 24],  # Column 0
        [3, 6, 13, 16, 23],  # Column 1
        [2, 7, 12, 17, 22],  # Column 2
        [1, 8, 11, 18, 21],  # Column 3
        [0, 9, 10, 19, 20]   # Column 4
    ]

    # Determine the column based on frequency
    if freq < 131:
        column = 0
    elif freq <= 261:
        column = 1
    elif freq < 521:
        column = 2
    elif freq < 1041:
        column = 3
    elif freq < 4187:
        column = 4
    else:
        column = -1  # Out of range

    # Determine intensity based on duration
    if duration < 126:
        intensity = 0
    elif duration < 251:
        intensity = 1
    elif duration < 501:
        intensity = 2
    elif duration < 1001:
        intensity = 3
    else:
        intensity = -1

    if column == -1 or intensity == -1:
        return  # Ignore invalid values

    # Define the color pattern for each intensity level
    color_patterns = [
        [BLUE],  # Intensity 0
        [BLUE, MAGENTA, MAGENTA],  # Intensity 1
        [BLUE, MAGENTA, MAGENTA, RED],  # Intensity 2
        [BLUE, MAGENTA, MAGENTA, RED, RED] # Intensity 3
    ]

    # # Clear the column
    # for i in range(5):
    #     np[i * 5 + column] = BLACK

    # Light up LEDs based on intensity level
    colors = color_patterns[intensity]
    for i in range(len(colors)):
        np[index[column][i]] = colors[i]

    np.write()

# Turn off LEDs
def clear_all():
    np.fill(BLACK)
    np.write()

# Main Player Loop
def music_player():
    global running, next_song
    current_song_index = 0

    display_menu()

    while True:
        if running:
            play_song(songs[current_song_index])

        if next_song:
            current_song_index = (current_song_index + 1) % len(songs)
            running = True
            next_song = False

        pause_song()  # Ensure sound stops if paused

        time.sleep(0.2)

music_player()

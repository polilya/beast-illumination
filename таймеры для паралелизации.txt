import machine
import neopixel
import time
import random

# Параметры подключения
PIN_WS2812B = 26  # Пин ESP32, подключенный к ленте WS2812B
NUM_PIXELS = 102  # Общее количество светодиодов

# Инициализация светодиодной ленты
np = neopixel.NeoPixel(machine.Pin(PIN_WS2812B), NUM_PIXELS)

# Параметр скорости (чем больше, тем быстрее)
speedFactor = 40
UPDATE_INTERVAL = 1.0 / speedFactor  # Интервал обновления в секундах
FADE_DURATION = 5.0 / speedFactor    # Время полного затухания (адаптировано под speedFactor)
BLINK_INTERVAL = 1               # Интервал случайного мерцания
active_pixels = {}                    # Храним состояние пикселей {номер: (R, G, B, время включения)}

# Определение цветовых параметров (желтый, оранжевый, красный)
FIRE_COLORS = [
    (255, 100, 0),  # Оранжево-желтый
    (255, 50, 0),   # Оранжевый
    (200, 50, 0),   # Красно-оранжевый
    (255, 0, 0),    # Красный
]

# Исходный фиолетовый цвет (RGB)
baseR, baseG, baseB = 148, 0, 211
# Бирюзовый цвет (RGB)
targetR, targetG, targetB = 0, 255, 255
colorShiftFactor = 0.05

# Функция получения промежуточного цвета
def get_gradient_color(step):
    r = int(baseR + (targetR - baseR) * step * colorShiftFactor)
    g = int(baseG + (targetG - baseG) * step * colorShiftFactor)
    b = int(baseB + (targetB - baseB) * step * colorShiftFactor)
    return (max(0, min(r, 255)), max(0, min(g, 255)), max(0, min(b, 255)))

# Функция для установки пикселя с учетом времени включения
def set_pixel_with_fade(index, color, current_time):
    active_pixels[index] = (color[0], color[1], color[2], current_time)
    np[index] = color
    np.write()

# Функция для обновления состояния всех пикселей
def update_pixels(current_time):
    for pixel, (r, g, b, start_time) in list(active_pixels.items()):
        elapsed_time = current_time - start_time
        if elapsed_time >= FADE_DURATION:
            np[pixel] = (0, 0, 0)  # Полное затухание
            del active_pixels[pixel]
        else:
            fade_factor = max(0, 1 - (elapsed_time / FADE_DURATION))
            np[pixel] = (
                int(r * fade_factor),
                int(g * fade_factor),
                int(b * fade_factor),
            )
    np.write()

# Функция интенсивного огненного мерцания (только для группы 0-41)
def fire_effect(current_time):
    if random.random() < BLINK_INTERVAL:
        fire_group = range(0, 42)
        pixel = random.choice(list(fire_group))
        color = random.choice(FIRE_COLORS)
        set_pixel_with_fade(pixel, color, current_time)

# Функция плавного включения волны (поочередно группами)
def wave_on(current_time):
    for i in range(16):
        if (67 - i) >= 0:
            set_pixel_with_fade(67 - i, get_gradient_color(i), current_time)
        if (67 + i) < NUM_PIXELS:
            set_pixel_with_fade(67 + i, get_gradient_color(i), current_time)
        time.sleep(1.0 / speedFactor)

    for i in range(10):
        if (83 + i) < NUM_PIXELS:
            set_pixel_with_fade(83 + i, get_gradient_color(i + 20), current_time)
        if (92 + i) < NUM_PIXELS:
            set_pixel_with_fade(92 + i, get_gradient_color(i + 20), current_time)
        if (51 - i) >= 0:
            set_pixel_with_fade(51 - i, get_gradient_color(i + 20), current_time)
        time.sleep(1.0 / speedFactor)

# Основной цикл работы с единой системой времени
start_time = time.time()
state = "on"

while True:
    current_time = time.time() - start_time

    if state == "on":
        wave_on(current_time)
        state = "fire"
    elif state == "fire":
        fire_effect(current_time)
        if current_time > 5:  # После 5 секунд переходим к выключению
            state = "off"
    elif state == "off":
        if not active_pixels:  # Ждем пока все пиксели потухнут
            start_time = time.time()
            state = "on"

    update_pixels(current_time)
    time.sleep(UPDATE_INTERVAL)
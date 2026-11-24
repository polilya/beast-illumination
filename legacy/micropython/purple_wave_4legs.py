import machine
import neopixel
import time
import random

# Параметры подключения
LED_PINS = [26, 25, 33, 32]  # Пины ESP32, подключенные к разным лентам WS2812B
NUM_PIXELS = 102             # Общее количество светодиодов в каждой ленте
phaseShift = 50             # Разность фаз в миллисекундах между диодами

# Инициализация всех светодиодных лент
led_strips = [neopixel.NeoPixel(machine.Pin(pin), NUM_PIXELS) for pin in LED_PINS]

# Исходный фиолетовый цвет (RGB)
baseR, baseG, baseB = 148, 0, 211
# Бирюзовый цвет (RGB)
targetR, targetG, targetB = 0, 255, 255

# Коэффициент изменения цвета
colorShiftFactor = 0.1

# Параметр скорости (чем больше, тем быстрее)
speedFactor = 20

# Функция для получения промежуточного цвета
def get_gradient_color(step):
    r = int(baseR + (targetR - baseR) * step * colorShiftFactor)
    g = int(baseG + (targetG - baseG) * step * colorShiftFactor)
    b = int(baseB + (targetB - baseB) * step * colorShiftFactor)

    r = max(0, min(r, 255))
    g = max(0, min(g, 255))
    b = max(0, min(b, 255))

    return (r, g, b)

# Функция плавного включения с фазовым сдвигом
def wave_on_all():
    start_time = time.ticks_ms()
    
    while time.ticks_diff(time.ticks_ms(), start_time) < (16 + 10) * phaseShift:
        elapsed = time.ticks_diff(time.ticks_ms(), start_time)

        for strip in led_strips:
            for i in range(16):
                if elapsed >= i * phaseShift:
                    if (67 - i) >= 0:
                        strip[67 - i] = get_gradient_color(i)
                    if (67 + i) < NUM_PIXELS:
                        strip[67 + i] = get_gradient_color(i)
                    strip.write()
            
            for i in range(10):
                if elapsed >= (16 + i) * phaseShift:
                    if (83 + i) < NUM_PIXELS:
                        strip[83 + i] = get_gradient_color(i + 20)
                    if (93 + i) < NUM_PIXELS:
                        strip[93 + i] = get_gradient_color(i + 20)
                    if (51 - i) >= 0:
                        strip[51 - i] = get_gradient_color(i + 20)
                    strip.write()
        time.sleep(0.05)

# Функция случайного мерцания
def random_blink_all():
    start_time = time.ticks_ms()
    
    while time.ticks_diff(time.ticks_ms(), start_time) < 3000:  # Мигание в течение 3 секунд
        for strip in led_strips:
            random_pixel = random.randint(0, 41)
            strip[random_pixel] = (0, 255, random.randint(128, 255))
            strip.write()
        time.sleep(random.uniform(0.05, 0.1))

# Функция плавного выключения с фазовым сдвигом
def wave_off_all():
    start_time = time.ticks_ms()
    
    while time.ticks_diff(time.ticks_ms(), start_time) < (16 + 10) * phaseShift:
        elapsed = time.ticks_diff(time.ticks_ms(), start_time)

        for strip in led_strips:
            for i in range(9, -1, -1):
                if elapsed >= i * phaseShift:
                    if (83 + i) < NUM_PIXELS:
                        strip[83 + i] = (0, 0, 0)
                    if (93 + i) < NUM_PIXELS:
                        strip[93 + i] = (0, 0, 0)
                    if (51 - i) >= 0:
                        strip[51 - i] = (0, 0, 0)
                    strip.write()

            for i in range(15, -1, -1):
                if elapsed >= (10 + i) * phaseShift:
                    if (67 - i) >= 0:
                        strip[67 - i] = (0, 0, 0)
                    if (67 + i) < NUM_PIXELS:
                        strip[67 + i] = (0, 0, 0)
                    strip.write()
        time.sleep(0.05)

# Основной цикл работы
while True:
    wave_on_all()       # Одновременно включаем все ленты с фазовым сдвигом
    random_blink_all()  # Случайное мерцание
    wave_off_all()      # Одновременно выключаем все ленты с фазовым сдвигом

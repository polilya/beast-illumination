import machine
import neopixel
import time

# Подключение к ленте
PIN_WS2812B = 26  # Пин ESP32
NUM_PIXELS = 102  # Количество светодиодов
np = neopixel.NeoPixel(machine.Pin(PIN_WS2812B), NUM_PIXELS)

# Цвета
BASE_COLOR = (50, 0, 0)     # Тёмно-красный (фон)
PEAK_COLOR = (255, 50, 0)   # Яркий красно-оранжевый (вспышка)

# Группы светодиодов
TOP_TRIANGLE = list(range(0, 30))   # Верхний треугольник (0-29)
MAIN_LINE_DOWN = list(range(30, 67))  # Вертикальная линия вниз (30-66)
MAIN_LINE_UP = list(range(67, 92))    # Вертикальная линия вверх (67-91)
PARALLEL_GROUP = list(range(92, 102)) # Дополнительные (92-101)

ALL_PIXELS = TOP_TRIANGLE + MAIN_LINE_DOWN + MAIN_LINE_UP + PARALLEL_GROUP  # Все пиксели

# Настройка ритма сердцебиения (каждое число – скорость одного удара)
HEARTBEAT_PATTERN = [0.6, 0.6, 1.2]  # Например: два быстрых удара + один медленный

# Функция для плавного изменения яркости
def pulse_effect(speed=1.0):
    """Плавное увеличение и уменьшение яркости для имитации сердечного удара"""

    rise_time = 0.01 * speed  # Скорость всплеска
    fall_time = 0.02 * speed   # Скорость затухания

    for brightness in range(0, 101, 5):  # Наращивание яркости
        scale = brightness / 100
        color = (
            int(BASE_COLOR[0] + (PEAK_COLOR[0] - BASE_COLOR[0]) * scale),
            int(BASE_COLOR[1] + (PEAK_COLOR[1] - BASE_COLOR[1]) * scale),
            int(BASE_COLOR[2] + (PEAK_COLOR[2] - BASE_COLOR[2]) * scale),
        )
        for pixel in ALL_PIXELS:
            np[pixel] = color
        np.write()
        time.sleep(rise_time)

    for brightness in range(100, -1, -5):  # Затухание
        scale = brightness / 100
        color = (
            int(BASE_COLOR[0] + (PEAK_COLOR[0] - BASE_COLOR[0]) * scale),
            int(BASE_COLOR[1] + (PEAK_COLOR[1] - BASE_COLOR[1]) * scale),
            int(BASE_COLOR[2] + (PEAK_COLOR[2] - BASE_COLOR[2]) * scale),
        )
        for pixel in ALL_PIXELS:
            np[pixel] = color
        np.write()
        time.sleep(fall_time)


# Основной цикл сердцебиения
while True:
    for speed in HEARTBEAT_PATTERN:
        pulse_effect(speed)  
        time.sleep(0.08 * speed)  # Короткая пауза между ударами
    time.sleep(0.4)  # Длинная пауза перед новым циклом

import machine
import neopixel
import time
import random

# Пины для 4 ног
LED_PINS = [2, 4, 5, 12, 13, 14, 15, 18, 19, 21, 22, 23]  # 12 GPIO-пинов
NUM_PIXELS = 102  # Количество светодиодов на каждую ногу

# Создаём объекты NeoPixel для каждой ноги
legs = [neopixel.NeoPixel(machine.Pin(pin), NUM_PIXELS) for pin in LED_PINS]

# Настройки эффекта
DISCO_SPEED = 0.1  # Скорость смены цветов (чем меньше, тем быстрее)
BRIGHTNESS = 150  # Максимальная яркость (0-255)

def get_random_color():
    """Генерирует случайный яркий цвет"""
    return (
        random.randint(50, BRIGHTNESS),  # Красный
        random.randint(50, BRIGHTNESS),  # Зелёный
        random.randint(50, BRIGHTNESS)   # Синий
    )

def disco_crab():
    """Эффект диско-краба - хаотичная смена цветов для 4 ног"""
    while True:
        for leg in legs:  # Проходим по каждой ноге
            for i in range(NUM_PIXELS):  
                leg[i] = get_random_color()  # Устанавливаем случайный цвет
            leg.write()  # Применяем изменения
        time.sleep(DISCO_SPEED)  # Скорость обновления анимации

# Запуск эффекта
disco_crab()

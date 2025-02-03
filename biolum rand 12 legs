import machine
import neopixel
import time
import random

# Пины для каждой из 12 ног
LED_PINS = [2, 4, 5, 12, 13, 14, 15, 18, 19, 21, 22, 23]  # 12 GPIO-пинов
NUM_PIXELS = 102  # Количество светодиодов на каждую ногу

# Создаём объекты NeoPixel для каждой ноги
legs = [neopixel.NeoPixel(machine.Pin(pin), NUM_PIXELS) for pin in LED_PINS]

# Настройки эффекта
SPARKLE_PROBABILITY = 0.08  # Шанс вспышки (чем больше, тем больше искр)
DECAY_SPEED = 0.05  # Скорость затухания
UPDATE_SPEED = 0.05  # Скорость обновления анимации

# Храним текущую яркость пикселей каждой ноги
current_brightness = [[0] * NUM_PIXELS for _ in LED_PINS]

def get_bioluminescent_color(brightness):
    """Генерирует случайные сине-зелёные цвета с разной интенсивностью"""
    return (
        0,  # Красный = 0 (морская тематика)
        int(30 + 225 * brightness),  # Зелёный (от слабого до яркого)
        int(50 + 205 * brightness)   # Синий (от слабого до яркого)
    )

def update_leg(leg, brightness_array):
    """Обновляет свечение одной ноги"""
    for i in range(NUM_PIXELS):
        if random.random() < SPARKLE_PROBABILITY:  # Новая случайная вспышка
            brightness_array[i] = 1.0  # Максимальная яркость
        
        # Плавное затухание
        if brightness_array[i] > 0:
            brightness_array[i] -= DECAY_SPEED  # Уменьшаем яркость
            if brightness_array[i] < 0:
                brightness_array[i] = 0  # Не уходим в отрицательные значения
        
        # Устанавливаем цвет пикселя в зависимости от его яркости
        leg[i] = get_bioluminescent_color(brightness_array[i])

    leg.write()

def bioluminescence_effect():
    """Запускает биолюминесценцию для всех 12 ног одновременно"""
    while True:
        for i, leg in enumerate(legs):
            update_leg(leg, current_brightness[i])  # Обновляем каждую ногу
        time.sleep(UPDATE_SPEED)

# Запуск эффекта
bioluminescence_effect()

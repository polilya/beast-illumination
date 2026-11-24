import machine
import neopixel
import time
import random
import math

# Пины для 4 ног
LED_PINS = [2, 4, 5, 12, 13, 14, 15, 18, 19, 21, 22, 23]  # 12 GPIO-пинов
NUM_PIXELS = 102  # Количество светодиодов на каждую ногу

# Создаём объекты NeoPixel для каждой ноги
legs = [neopixel.NeoPixel(machine.Pin(pin), NUM_PIXELS) for pin in LED_PINS]

# Настройки эффекта
BREATH_SPEED = 0.02  # Скорость дыхания (чем меньше, тем медленнее)
SPASM_PROBABILITY = 0.005  # Вероятность случайного "спазма" в ноге
NEURON_FLICKER_PROB = 0.02  # Шанс короткого мерцания отдельного пикселя
BASE_COLOR = (0, 50, 100)  # Основной цвет (спокойный голубой)
PEAK_COLOR = (0, 255, 255)  # Цвет максимального свечения
SPASM_COLOR = (255, 0, 0)  # Цвет резкой вспышки ("спазма")

def fade_between_colors(start_color, end_color, factor):
    """Плавное изменение цвета между двумя значениями (factor от 0 до 1)"""
    return tuple(
        int(start_color[i] + (end_color[i] - start_color[i]) * factor)
        for i in range(3)
    )

def breathing_effect(t):
    """Генерирует плавный коэффициент для дыхания"""
    return (math.sin(t * 0.1) + 1) / 2  # Значение от 0 до 1

def alive_effect():
    """Основной эффект - имитация живого организма"""
    t = 0  # Временная переменная для дыхания

    while True:
        breath_factor = breathing_effect(t)  # Вычисляем дыхательный коэффициент
        color = fade_between_colors(BASE_COLOR, PEAK_COLOR, breath_factor)  # Цвет для дыхания

        for leg in legs:
            for i in range(NUM_PIXELS):
                # Случайное мерцание (имитация нейронных вспышек)
                if random.random() < NEURON_FLICKER_PROB:
                    leg[i] = (255, 255, 255)  # Короткая вспышка белого света
                else:
                    leg[i] = color  # Основное дыхание

            # Случайный "спазм" (одна нога вспыхивает резко)
            if random.random() < SPASM_PROBABILITY:
                for i in range(NUM_PIXELS):
                    leg[i] = SPASM_COLOR  # Вспышка красного цвета

            leg.write()  # Обновляем ногу

        t += BREATH_SPEED  # Увеличиваем время (управляет дыханием)
        time.sleep(BREATH_SPEED)

# Запуск эффекта
alive_effect()

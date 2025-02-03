import machine
import neopixel
import time
import random

# Пины для каждой ноги
LED_PINS = [2, 4, 5, 12, 13, 14, 15, 18, 19, 21, 22, 23]  # 12 GPIO-пинов
NUM_PIXELS = 102  # Количество светодиодов на каждую ногу

# Создаём объекты NeoPixel для каждой ноги
legs = [neopixel.NeoPixel(machine.Pin(pin), NUM_PIXELS) for pin in LED_PINS]

# Настройки эффекта
BRIGHTNESS_STEPS = 20  # Количество шагов для плавного перехода
RES_SPEED = 0.02  # Скорость изменения яркости (чем меньше, тем быстрее)
BASE_COLOR = (10, 0, 30)  # Тёмный синий (фон)
GLOW_COLOR = (0, 100, 255)  # Яркий голубой (свечения)

def fade_between_colors(start_color, end_color, step, max_steps):
    """Плавное изменение цвета между двумя значениями"""
    return tuple(
        int(start_color[i] + (end_color[i] - start_color[i]) * (step / max_steps))
        for i in range(3)
    )

def shuffle_list(lst):
    """Реализация перемешивания списка вручную (без shuffle)"""
    for i in range(len(lst) - 1, 0, -1):
        j = random.randint(0, i)
        lst[i], lst[j] = lst[j], lst[i]

def resonance_effect():
    """Эффект светового резонанса - ноги загораются в случайном порядке"""
    while True:
        leg_order = legs[:]  # Копируем список ног
        shuffle_list(leg_order)  # Перемешиваем порядок ног вручную

        for leg in leg_order:
            # Плавное зажигание
            for step in range(BRIGHTNESS_STEPS):
                color = fade_between_colors(BASE_COLOR, GLOW_COLOR, step, BRIGHTNESS_STEPS)
                for i in range(NUM_PIXELS):
                    leg[i] = color
                leg.write()
                time.sleep(RES_SPEED)

            # Плавное затухание
            for step in range(BRIGHTNESS_STEPS, -1, -1):
                color = fade_between_colors(BASE_COLOR, GLOW_COLOR, step, BRIGHTNESS_STEPS)
                for i in range(NUM_PIXELS):
                    leg[i] = color
                leg.write()
                time.sleep(RES_SPEED)

# Запуск эффекта
resonance_effect()

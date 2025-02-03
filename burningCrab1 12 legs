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

# Определяем группы светодиодов в ноге
def get_leg_groups():
    return {
        "TOP_TRIANGLE": list(range(0, 30)),   # Верхний треугольник (0-29)
        "MAIN_LINE_DOWN": list(range(30, 67)),  # Линия вниз (30-66)
        "MAIN_LINE_UP": list(range(67, 92)),    # Линия вверх (67-91)
        "PARALLEL_GROUP": list(range(92, 102))  # Параллельная группа (92-101)
    }

# Группы ног
leg_groups = [get_leg_groups() for _ in range(len(LED_PINS))]

# Настройки эффекта
FLICKER_PROBABILITY = 0.02  # Вероятность случайного мерцания (искр)
WAVE_PROBABILITY = 0.005  # Вероятность запуска волны
WAVE_SPEED = 0.05  # Скорость распространения волны
FIRE_COLORS = [(255, 50, 0), (255, 100, 0), (255, 150, 50), (255, 200, 100)]  # Красно-оранжевые цвета
BASE_COLOR = (30, 0, 0)  # Тёмно-красный фоновый цвет

# Храним активные волны
active_waves = []

def fade_between_colors(start_color, end_color, factor):
    """Плавное изменение цвета между двумя значениями (factor от 0 до 1)"""
    return tuple(
        int(start_color[i] + (end_color[i] - start_color[i]) * factor)
        for i in range(3)
    )

def fire_flicker():
    """Случайное мерцание отдельных пикселей"""
    for leg in legs:
        for i in range(NUM_PIXELS):
            if random.random() < FLICKER_PROBABILITY:
                leg[i] = random.choice(FIRE_COLORS)  # Искры случайного цвета
        leg.write()

def fire_wave():
    """Запускает огненную волну в случайной ноге и группе"""
    if random.random() < WAVE_PROBABILITY:
        leg_index = random.randint(0, len(legs) - 1)  # Выбираем случайную ногу
        group_name = random.choice(list(leg_groups[leg_index].keys()))  # Выбираем случайную группу
        start_pixel = random.choice(leg_groups[leg_index][group_name])  # Выбираем стартовый пиксель волны
        active_waves.append({"leg": leg_index, "pixels": [start_pixel], "step": 0})

def update_waves():
    """Обновляет активные волны, распространяя их наружу"""
    for wave in active_waves:
        leg = legs[wave["leg"]]
        new_pixels = []
        for pixel in wave["pixels"]:
            if 0 <= pixel < NUM_PIXELS:
                color_index = min(wave["step"], len(FIRE_COLORS) - 1)  # Выбираем цвет из градиента
                leg[pixel] = FIRE_COLORS[color_index]  # Устанавливаем цвет
                # Добавляем соседние пиксели для расширения волны
                if pixel - 1 >= 0:
                    new_pixels.append(pixel - 1)
                if pixel + 1 < NUM_PIXELS:
                    new_pixels.append(pixel + 1)
        wave["pixels"] = list(set(new_pixels))  # Убираем дубликаты пикселей
        wave["step"] += 1  # Увеличиваем шаг волны
        leg.write()
    
    # Удаляем старые волны
    active_waves[:] = [wave for wave in active_waves if wave["step"] < len(FIRE_COLORS)]

def fire_effect():
    """Основной эффект - хаотичные вспышки и расходящиеся волны"""
    while True:
        fire_flicker()  # Запускаем случайное мерцание (искры)
        fire_wave()  # Запускаем случайные волны
        update_waves()  # Обновляем распространяющиеся волны
        time.sleep(WAVE_SPEED)

# Запуск эффекта
fire_effect()

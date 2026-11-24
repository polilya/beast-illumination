import machine
import neopixel
import time
import random

# Пины для 4 ног
LED_PINS = [2, 4, 5, 12, 13, 14, 15, 18, 19, 21, 22, 23]  # 12 GPIO-пинов # GPIO для каждой ноги
NUM_PIXELS = 102  # Количество светодиодов на каждой ноге

# Создаём объекты NeoPixel для каждой ноги
legs = [neopixel.NeoPixel(machine.Pin(pin), NUM_PIXELS) for pin in LED_PINS]

# Настройки огня
WAVE_SPEED = 0.08  # Скорость обновления анимации
FLICKER_PROBABILITY = 0.82  # Шанс случайной вспышки
MAX_WAVES = 19  # Максимальное количество активных волн на каждую ногу

# Градиент огня (по мере угасания)
FIRE_COLORS = [
    (0, 0, 0),       #  Нет огня
    (50, 0, 0),      #  Тёмно-красный (слабое тление)
    (150, 30, 0),    #  Красно-оранжевый (разгорается)
    (255, 80, 0),    #  Яркий оранжевый (пик огня)
    (255, 150, 0),   #  Ослабевающий огонь
]

# Для каждой ноги создаём свой список волн
fire_waves = [[] for _ in range(len(legs))]

def clear_pixels():
    """Гасит все пиксели перед стартом"""
    for leg in legs:
        for i in range(NUM_PIXELS):
            leg[i] = (0, 0, 0)
        leg.write()

clear_pixels()

def start_new_wave(leg_index):
    """Создаёт новую волну на указанной ноге"""
    if len(fire_waves[leg_index]) < MAX_WAVES and random.random() < FLICKER_PROBABILITY:
        start_pos = random.randint(0, NUM_PIXELS - 1)  # Случайный пиксель
        fire_waves[leg_index].append({
            "center": start_pos,
            "radius": 0,
            "max_radius": random.randint(6, 18),  # Максимальный размер волны
            "intensity": len(FIRE_COLORS) - 1
        })

def update_waves():
    """Обновляет волны, расширяя их и уменьшая яркость"""
    for leg_index in range(len(legs)):
        for wave in fire_waves[leg_index]:
            wave["radius"] += 1
            wave["intensity"] = max(1, wave["intensity"] - 1)

        # Удаляем завершённые волны
        fire_waves[leg_index] = [wave for wave in fire_waves[leg_index] if wave["radius"] <= wave["max_radius"]]

def apply_fire():
    """Применяет огонь ко всем ногам"""
    for leg_index, leg in enumerate(legs):
        fire_map = [0] * NUM_PIXELS  # Очистка карты огня

        for wave in fire_waves[leg_index]:
            center = wave["center"]
            radius = wave["radius"]
            intensity = wave["intensity"]

            for i in range(NUM_PIXELS):
                distance = abs(i - center)
                if distance <= radius:
                    fire_map[i] = max(fire_map[i], intensity - (distance // 2))

        # Обновляем цвета светодиодов
        for i in range(NUM_PIXELS):
            level = min(fire_map[i], len(FIRE_COLORS) - 1)
            leg[i] = FIRE_COLORS[level]

        leg.write()

def fire_effect():
    """Основной цикл эффекта"""
    while True:
        for leg_index in range(len(legs)):
            start_new_wave(leg_index)  # Добавляем волны
        update_waves()  # Обновляем все волны
        apply_fire()  # Применяем эффект ко всем ногам
        time.sleep(WAVE_SPEED)

# Запуск эффекта
fire_effect()

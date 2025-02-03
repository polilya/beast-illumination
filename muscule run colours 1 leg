import machine
import neopixel
import time
import random

# Подключение к ленте
PIN_WS2812B = 26  # Пин ESP32
NUM_PIXELS = 102  # Количество светодиодов
np = neopixel.NeoPixel(machine.Pin(PIN_WS2812B), NUM_PIXELS)

# Группы светодиодов
TOP_TRIANGLE = list(range(0, 30))   # Верхний треугольник (0-29)
MAIN_LINE_DOWN = list(range(30, 67))  # Вертикальная линия вниз (30-66)
MAIN_LINE_UP = list(range(67, 92))    # Вертикальная линия вверх (67-91)
PARALLEL_GROUP = list(range(92, 102)) # Дополнительные (92-101)

GROUPS = [TOP_TRIANGLE, MAIN_LINE_DOWN, MAIN_LINE_UP, PARALLEL_GROUP]  # Все группы

# Настройки эффекта
STRETCH_LENGTH = 10   # Длина области растяжения
STRETCH_SPEED = 0.02  # Скорость движения волны
WAVE_DELAY = 3        # Разница фаз между группами

# Создаём массив текущих цветов каждого пикселя (чтобы цвета сохранялись)
current_colors = [(0, 0, 0)] * NUM_PIXELS

def get_random_color():
    """Возвращает случайный цвет"""
    return (
        random.randint(100, 255),  # Красный (избегаем слишком тёмных цветов)
        random.randint(100, 255),  # Зелёный
        random.randint(100, 255)   # Синий
    )

def muscle_stretch():
    """Эффект растяжения мышц с постоянной сменой цвета"""
    
    while True:
        NEW_WAVE_COLOR = get_random_color()  # Выбираем новый случайный цвет

        for step in range(max(len(group) for group in GROUPS) + STRETCH_LENGTH):
            for group_index, group in enumerate(GROUPS):
                for j, pixel in enumerate(group):
                    offset = group_index * WAVE_DELAY  # Смещение волны между группами
                    distance = abs((step - offset) - j)
                    
                    if distance < STRETCH_LENGTH:
                        # Чем ближе пиксель к центру растяжения, тем он ярче
                        scale = 1 - (distance / STRETCH_LENGTH)
                        new_color = (
                            int(current_colors[pixel][0] + (NEW_WAVE_COLOR[0] - current_colors[pixel][0]) * scale),
                            int(current_colors[pixel][1] + (NEW_WAVE_COLOR[1] - current_colors[pixel][1]) * scale),
                            int(current_colors[pixel][2] + (NEW_WAVE_COLOR[2] - current_colors[pixel][2]) * scale),
                        )
                        current_colors[pixel] = new_color  # Обновляем цвет пикселя
                    else:
                        # Пиксель сохраняет свой цвет
                        new_color = current_colors[pixel]

                    np[pixel] = new_color

            np.write()
            time.sleep(STRETCH_SPEED)

# Запуск анимации
muscle_stretch()

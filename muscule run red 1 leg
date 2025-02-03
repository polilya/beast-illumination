import machine
import neopixel
import time

# Подключение к ленте
PIN_WS2812B = 26  # Пин ESP32
NUM_PIXELS = 102  # Количество светодиодов
np = neopixel.NeoPixel(machine.Pin(PIN_WS2812B), NUM_PIXELS)

# Цвета для эффекта
RELAX_COLOR = (50, 0, 0)     # Тёмно-красный (расслабление)
STRETCH_COLOR = (255, 50, 0) # Яркий красно-оранжевый (напряжение)

# Группы светодиодов
TOP_TRIANGLE = list(range(0, 30))   # Верхний треугольник (0-29)
MAIN_LINE_DOWN = list(range(30, 67))  # Вертикальная линия вниз (30-66)
MAIN_LINE_UP = list(range(67, 92))    # Вертикальная линия вверх (67-91)
PARALLEL_GROUP = list(range(92, 102)) # Дополнительные (92-101)

GROUPS = [TOP_TRIANGLE, MAIN_LINE_DOWN, MAIN_LINE_UP, PARALLEL_GROUP]  # Все группы

# Настройки эффекта
STRETCH_LENGTH = 10  # Длина области растяжения
STRETCH_SPEED = 0.02  # Скорость растяжения
WAVE_DELAY = 3  # Разница фаз между группами (чем больше, тем сильнее сдвиг)

def muscle_stretch():
    """Эффект растяжения мышц с независимыми волнами в каждой группе"""
    
    for step in range(max(len(group) for group in GROUPS) + STRETCH_LENGTH):
        for group_index, group in enumerate(GROUPS):
            for j, pixel in enumerate(group):
                # Сдвиг волны между группами
                offset = group_index * WAVE_DELAY  
                distance = abs((step - offset) - j)
                
                if distance < STRETCH_LENGTH:
                    # Чем ближе пиксель к центру растяжения, тем он ярче
                    scale = 1 - (distance / STRETCH_LENGTH)
                    color = (
                        int(RELAX_COLOR[0] + (STRETCH_COLOR[0] - RELAX_COLOR[0]) * scale),
                        int(RELAX_COLOR[1] + (STRETCH_COLOR[1] - RELAX_COLOR[1]) * scale),
                        int(RELAX_COLOR[2] + (STRETCH_COLOR[2] - RELAX_COLOR[2]) * scale),
                    )
                else:
                    color = RELAX_COLOR

                np[pixel] = color

        np.write()
        time.sleep(STRETCH_SPEED)

# Основной цикл анимации
while True:
    muscle_stretch()
    time.sleep(0.1)  # Пауза между циклами (можно убрать для бесконечного движения)

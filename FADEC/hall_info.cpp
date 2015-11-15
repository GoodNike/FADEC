/**
* @copyright   Николай Крашенинников
* @project     Измерительный комплекс СУ
* @file        hall_info.cpp
* @brief       Класс для работы с датчиком Холла, файл реализации.
*/

#include "hall_info.h"

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "library.h"

static double m_speed;                  ///< Скорость вращения моментальная, оборотов в минуту.
static unsigned long m_rotates;         ///< Общее количество оборотов.
static unsigned long m_t1;              ///< Время фиксирования прошлого оборота, мс.
static unsigned long m_t2;              ///< Время фиксирования текущего оборота, мс.
static bool m_started;                  ///< Запущена ли уже работа с датчиком Холла?
static int m_pin;                       ///< Пин, к которому подсоединен датчик Холла.

/**
 * Обработчик прерывания от датчика Холла.
 * При фиксировании обработчиком очередного срабатывания датчика Холла сигнализирует об этом
 * остальным переменным класса путем изменения состояния переменно #m_calc и обновляет поля #m_t1,
 * #m_t2 и #m_rotates.
 */
static void hall_isr();

/**
 * Конструктор класса.
 */
hall_info::hall_info(int pin)
{
    if (!m_started) {
        m_pin = pin;
    }
}

/**
 * Запуск работы с датчиком Холла.
 */
bool hall_info::start()
{
    if (m_started) {
        return false;
    }
    pinMode(m_pin, INPUT_PULLUP);
    m_speed = 0;
    m_t1 = millis();
    m_t2 = millis();
    attachInterrupt(digitalPinToInterrupt(m_pin), hall_isr, FALLING);
    m_started = true;
    return true;
}

/**
 * Возврат текущей подсчитанной скорости вращения, оборотов в минуту.
 */
double hall_info::get_speed()
{
    return m_speed;
}

/**
 * Возврат общего количества оборотов с момента запуска #start().
 */
unsigned long hall_info::get_rotates()
{
    return m_rotates;
}

/**
 * Обработчик прерывания от датчика Холла.
 */
static void hall_isr()
{
    // Минимальное время между срабатываниями датчика Холла должно быть 1мс для исключения дребезга
    // датчика в момент контакта с магнитом. Соответственно максимальная фиксируемая скорость
    // вращения составляет 60000 об/мин.
    if (millis() - m_t2 >= 1) {
        m_rotates++;
        m_t1 = m_t2;
        m_t2 = millis();

        double velocity = 1000.0 / (m_t2 - m_t1) * 60.0;
        m_speed = l_apzveno_mp(velocity, m_speed, (m_t2 - m_t1) / 1000.0, 2.0);
    }
}

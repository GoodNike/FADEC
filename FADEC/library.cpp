/**
 * @copyright   Николай Крашенинников
 * @project     Измерительный комплекс СУ
 * @file        library.cpp
 * @brief       Библиотека дополнительных подпрограмм, файл реализации.
 */

#include "library.h"

/**
 * Прием символа RS232 от пользователя.
 */
byte l_getch()
{
    // Дожидаемся пока в буфере приема что-то появится.
    while (Serial.available() == 0);
    return (byte) Serial.read();
}

/**
 * Просмотр буфера приема RS232 на наличие данных без их извлечения.
 */
bool l_kbhit()
{
    return Serial.available() != 0;
}

/**
 * Прием вещественного числа по RS232 от пользователя.
 */
double l_scanf_double()
{
    long integer = 0;       ///< Целая часть числа.
    double frac = 0.0f;     ///< Дробная часть числа.
    double t = 0.1f;        ///< Переменная для определения места после запятой очередного вводимого числа.
    byte b;                 ///< Очередной введенный символ.
    bool point = false;     ///< Поставлена ли запятая.
    bool sign = false;      ///< Число отрицательно? По умолчанию число положительное.
    bool error = false;     ///< Ошибка ввода информации.
    double result;          ///< Итоговое число для возврата.

    while (true) {
        b = l_getch();
        // Выдадим в эхо, что ввел пользователь. Контроля выдачи производить не надо, т.к. контролируется
        // визуально. Какое должно быть поведение ПО в случае ошибки не ясно.
        Serial.write(b);

        // Введено число.
        if (b >= '0' && b <= '9') {
            if (!point) {
                // Если вводим до запятой.
                integer = integer * 10 + (b - '0');
            } else {
                // Если вводим после запятой.
                frac += (b - '0') * t;
                t *= 0.1f;
            }
        }
        // Введен символ разделения дробной и целой части.
        else if (b == '.' || b == ',') {
            if (!point) {
                point = true;
            } else {
                // Если повторно введен символ разграничения дробной и целой части - ошибка.
                error = true;
                break;
            }
        }
        // Введен отрицательный знак.
        else if (b == '-') {
            // Попытка смены знака посреди ввода числа - ошибка.
            if (integer != 0 || point) {
                error = true;
                break;
            }
            if (!sign) {
                sign = true;
            } else {
                // Попытка повторно сменить знак числа - ошибка.
                error = true;
                break;
            }
        }
        // Завершающий ввод символ.
        else if (b == '\r' || b == '\n') {
            // Конец ввода.
            break;
        }
        // Какой-либо другой символ - ошибка.
        else {
            error = true;
            break;
        }
    }

    if (!error) {
        result = integer + frac;
        // Для отрицательного числа сменим знак.
        if (sign) {
            result *= -1.0f;
        }
    } else {
        // Если ошибка вернем NAN (not-a-number).
        result = NAN;
    }

    return result;
}

/**
 * Фильтр вида W = 1/(Tp + 1) - "Апериодическое звено" (метод прямоугольников).
 */
double l_apzveno_mp(double x, double yp, double dt, double T)
{
    double a, b;

    a = dt / T;
    b = 1 - a;
    return a*x + b*yp;
}

/**
 * Фильтр вида "скользящее среднее".
 */
double l_average(double x, double *arr, size_t size)
{
    size_t i;
    double sum;

    sum = 0.0;
    for (i = (size - 1); i >= 1; i--) {
        arr[i] = arr[i - 1];
        sum += arr[i];
    }
    arr[0] = x;
    sum += arr[0];

    return sum / size;
}

/**
 * Установка начальных условий фильтра вида "скользящее среднее".
 */
void l_average_nu(double *arr, size_t size, double init)
{
    size_t i;

    for (i = 0; i < size; i++) {
        arr[i] = init;
    }
}

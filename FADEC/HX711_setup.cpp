/**
 * @copyright   Николай Крашенинников
 * @project     Измерительный комплекс СУ
 * @file        HX711_setup.cpp
 * @brief       Библиотека подпрограмм настройки тензодатчика HX711, файл реализации.
 */

#include "HX711_setup.h"

#include <EEPROM.h>

#include "library.h"
#include "crc32.h"

/**
 * Сохранение информации в EEPROM.
 */
bool EEPROM_save(double scale, long offset)
{
    unsigned long data[3];

    data[0] = *(unsigned long *)&scale;
    data[1] = *(unsigned long *)&offset;
    data[2] = crc32_calc((byte *)data, 2 * sizeof(unsigned long));

    for (size_t i = 0; i < sizeof(data); ++i) {
        // Для увеличения срока службы EEPROM записываем только тогда, когда данные отличаются от
        // того, что уже находится в EEPROM.
        EEPROM.update(i, ((byte *)data)[i]);
    }

    return true;
}

/**
 * Загрузка информации из EEPROM.
 */
bool EEPROM_load(double &scale, long &offset)
{
    unsigned long data[3];

    for (size_t i = 0; i < sizeof(data); ++i) {
        ((byte *)data)[i] = EEPROM.read(i);
    }

    if (crc32_check((byte *)data, sizeof(data))) {
        scale  = *((double *)&data[0]);
        offset = *((long *)&data[1]);
        return true;
    } else {
        return false;
    }
}

/**
 * Настройки тензодатчика: масштабировка и подсчет нейтральной точки ("веса тары").
 */
bool HX711_setup(HX711 &loadcell)
{
    double  scale;  ///< Масштабировка.
    long    offset; ///< Нейтральная точка ("вес тары").
    double  weight; ///< Какой вес приложен к тензодатчику (вводит пользователь), кг.

    Serial.println("Nastroyka tenzodat4ika: nastroyka masshtabirovki i neytralnoy to4ki.");
    Serial.println();
    Serial.println("Dat4ik dolzen nahoditsia v neytralnom polozenii (t.e. ne nagruzen).");
    Serial.println("Ubedites', 4to ves sniat, i nazmite lubuu knopku.");
    l_getch();

    // Выполним первоначальную масштабировку и настройку нейтральной точки.
    scale = loadcell.set_scale();
    offset = loadcell.tare();

    // С помощью пользователя уточним масштабировку. Примем от пользователя сколько на весах.
    Serial.println("Nagruzite dat4ik izvestnim vesom. Vvedite ves, kg: ");
    weight = l_scanf_double();

    // Считаем сколько выдает датчик и перемасштабируем используя известное значение веса.
    scale = loadcell.get_units(10) / weight;

    // Выставка полученной масштабировки и нейтральной точки.
    loadcell.set_scale(scale);
    loadcell.set_offset(offset);

    // Запись констант в EEPROM: при ошибке вернем false.
    if (EEPROM_save(scale, offset)) {
        Serial.println("Nastroyki sohraneni v EEPROM.");
        return true;
    } else {
        Serial.println("OSHIBKA sohraneniia nastroek v EEPROM.");
        Serial.println("V tekushem seanse budut ispol'zovatsia polu4ennie nastroyki.");
        return false;
    }
}

/**
 * Настройки тензодатчика, используя заранее известную масштабировку и величину нейтральной
 * точки ("веса тары").
 */
bool HX711_preload(HX711 &loadcell)
{
    double scale;   ///< Масштабировка.
    long offset;    ///< Нейтральная точка ("вес тары").

    // Если все хорошо загрузим настройки, иначе берем настройки по умолчанию.
    if (EEPROM_load(scale, offset)) {
        loadcell.set_scale(scale);
        loadcell.set_offset(offset);
        return true;
    } else {
        loadcell.set_scale(93723.34f);
        loadcell.set_offset(0x7EAE6E);
        return false;
    }
}

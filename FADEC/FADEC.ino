/**
 * @copyright   Николай Крашенинников
 * @project     Измерительный комплекс СУ
 * @file        FADEC.ino
 * @brief       Файл реализации точки страта проектов на Arduino: функции #setup() и #loop().
 *
 * $Revision: 75 $
 * $Author: Николай $
 * $Date:: 2015-11-09 08:08:17#$
 */

#include <EEPROM.h>

#include "HX711.h"
#include "HX711_setup.h"
#include "hall_info.h"
#include "crc32.h"
#include "library.h"

/**
 * Структура с информацией о работе цикла.
 */
struct loop_info_t {
    unsigned long cycles;       ///< Общее количество циклов.
    unsigned long time1;        ///< Время начала цикла, мкс.
    unsigned long time2;        ///< Время завершения цикла, мкс.
    unsigned long time_mcs;     ///< Время выполнения цикла, мкс.
    unsigned long min_mcs;      ///< Минимальный период между циклами, мкс.
    unsigned long max_mcs;      ///< Максимальный период между циклами, мкс.
    unsigned int c_olverflow;   ///< Счетчик превышения времени выполнения циклов, сколько всего раз наступило.
    unsigned long time_begin;   ///< Время старта предыдущего цикла, мкс.
    bool olverflow;             ///< Признак того, что цикл не уложился в отведенное время #T0.

    /**
     * Установка начальных условий, сброс отказов.
     *
     * @param[in] dt Время дискретности, необходимо для правильной выставки минимальных/максимальных
     *               периодов циклов.
     */
    void reset_loop(unsigned int dt) {
        olverflow = false;
        c_olverflow = 0;
        cycles = 0;
        min_mcs = dt * 100000;      // Значительно больше минимального времени.
        max_mcs = 0;                // Значительно меньше максимального: достаточно установить в 0.
    }
};

loop_info_t loop_info;              ///< Информация о работе цикла вычислений.
hall_info hall(2);                  ///< Датчик Холла, настроенный на работу со вторым цифровым контактом.
HX711 loadcell(A1, A0);             ///< Тензоданчик, настроенный на работу с контактами A1 и A0 Arduino.

/**
* @note Перерыв между чтением тензодатчика составляет ~75мс: это время датчик не возвращает сигнал готовности.
* @note Процедура чтения показаний тензодатчика когда он свободен занимает ~1мс.
* @note Порой при дисрктности 100мс тензодатчик оказывается занят ~150мс (1 раз на 10 минут).
*/
const unsigned int T0 = 200;        ///< Дискретность, мс.
const unsigned int HX_T0 = 150;     ///< Дискретность съема информации с тензодатчика, мс.
unsigned int hx_time;               ///< Переменная для определения времени, прошедшего с последнего опроса тензодатчика.

/**
 * Работа с консолью: реакция на нажатие кнопок пользователем.
 * Выводит информационные кадры консоли, кадры о состоянии Arduino, позволяет производить
 * настройку тензодатчика.
 *
 * @param[in] c Код нажатой клавиши.
 */
void disp_free(byte c);

/**
 * Настройка Arduino, обязательная функция.
 * Выполняет:
 * - настройку RS232;
 * - порта для работы со светодиодом;
 * - настройка тензодатчика.
 */
void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);

    // Предустановим необходимые поля для структуры, описывающей цикл счета.
    loop_info.reset_loop(T0);

    // Загружаем значения настроек из EEPROM.
    if (!HX711_preload(loadcell)) {
        Serial.println(F("Oshibka zagruzki nastoek tenzodatchika - trebuetsia nastroyka."));
        Serial.println(F("Ispol'zuutsia nastoyki po umol4aniu."));
    }

    // Запустим работу с датчиком Холла.
    hall.start();
}

/**
 * Основной цикл Arduino, обязательная функция.
 * Дискретность оперативно настраивается через #T0.
 * Выполняет:
 * - выводит на консоль показания тензодатчика;
 * - выполняет интерактивное взаимодействие с пользователем;
 * - управляет светодиодом: "моргание" 1 раз в секунду (1Гц).
 */
void loop()
{
    // Определение времени начала цикла, минимального и максимального периода.
    loop_info.time1 = micros();
    if (loop_info.cycles) {
        if (loop_info.time1 - loop_info.time_begin < loop_info.min_mcs) {
            loop_info.min_mcs = loop_info.time1 - loop_info.time_begin;
        }
        if (loop_info.time1 - loop_info.time_begin > loop_info.max_mcs) {
            loop_info.max_mcs = loop_info.time1 - loop_info.time_begin;
        }
    }
    loop_info.time_begin = loop_info.time1;

    // Съем информации с тензодатчика по необходимости: только если настало время его работы исходя
    // из его дискретности и общей дискртености.
    hx_time += T0;
    if (hx_time > HX_T0) {
        loadcell.get_units();
        hx_time = 0;
    }

    // Управление светодидом: поджигаем каждую четную секунду, гасим каждую нечетную.
    if ((loop_info.cycles * T0 / 1000) % 2) {
        digitalWrite(LED_BUILTIN, LOW);
    } else {
        digitalWrite(LED_BUILTIN, HIGH);
    }

    // Работа с консолью.
    if (l_kbhit()) {
        disp_free(l_getch());
    } else {
        disp_free('[');
    }

    // Определение окончания цикла, времени работы цикла.
    loop_info.cycles++;
    loop_info.time2 = micros();
    loop_info.time_mcs = loop_info.time2 - loop_info.time1;

    // По необходимости установим признак переполнения цикла по необходимости, ставится с запоминанием.
    if ((loop_info.time2 - loop_info.time1) / 1000 > T0) {
        loop_info.olverflow = true;
        loop_info.c_olverflow++;
    } else {
        delay(T0 - (loop_info.time2 - loop_info.time1) / 1000);
    }
}

/**
 * Работа с консолью: реакция на нажатие кнопок пользователем.
 */
void disp_free(byte c)
{
    switch (c) {

    // Кадр с основным постоянным выводом FADEC.
    case '[':
    {
        Serial.print(loop_info.cycles);     Serial.print(" : ");
        Serial.print(-loadcell.get_last_units()); Serial.print(" ");
        Serial.println(hall.get_speed());
    }
    break;

    // Кадр с информацией о работе FADEC.
    case '0':
    {
        Serial.println();
        int hours = loop_info.cycles/(1000/T0)/60/60;
        int minutes = (loop_info.cycles/(1000/T0)/60)%60;
        int seconds = (loop_info.cycles/(1000/T0))%60;

        Serial.print(F("Vremia rabori         : ")); Serial.print(hours); Serial.print(F("h ")); Serial.print(minutes); Serial.print(F("m ")); Serial.print(seconds); Serial.println(F("s"));
        Serial.print(F("Diskretnost'          : ")); Serial.println(T0);
        Serial.print(F("Obshee kol-vo ciklov  : ")); Serial.println(loop_info.cycles);
        Serial.print(F("Vremia vipolneniia, ms: ")); Serial.println(loop_info.time_mcs / 1000.0f);
        Serial.print(F("Priznak perepolneniia : "));
        if (loop_info.olverflow) {
            Serial.println(F("USTANOVLEN!"));
        } else {
            Serial.println(F("sniat"));
        }
        Serial.print(F("Cikli s perepolneniem : ")); Serial.println(loop_info.c_olverflow);
        Serial.print(F("Min period, ms        : ")); Serial.println(loop_info.min_mcs / 1000.0f);
        Serial.print(F("Max period, ms        : ")); Serial.println(loop_info.max_mcs / 1000.0f);
        Serial.println();
    }
    break;

    // Кадр с текущими настройками тензодатчика.
    case '1':
    {
        Serial.println();
        Serial.println(F("Pokazaniia tenzodat4ika:"));
        Serial.print(F("V masshtabe, kg      = ")); Serial.println(loadcell.get_units(), 5);
        Serial.print(F("Signal, hex          = ")); Serial.println(loadcell.read(), HEX);
        Serial.print(F("Signal - OFFSET, hex = ")); Serial.println(loadcell.get_value(), HEX);
        Serial.println();

        Serial.println(F("Tekushie nastroyki tenzodat4ika:"));
        Serial.print(F("scale  = ")); Serial.println(loadcell.get_scale(), 5);
        Serial.print(F("offset = ")); Serial.println(loadcell.get_offset(), HEX);
        Serial.println();

        Serial.println(F("Nastroyki v EEPROM: "));
        unsigned long data[3];
        for (size_t i = 0; i < sizeof(data); ++i) {
            ((byte *)data)[i] = EEPROM.read(i);
        }

        Serial.print(F("scale  = ")); Serial.println(*((double *)&data[0]), 5);
        Serial.print(F("scale, hex  = ")); Serial.println(*((long *)&data[0]), HEX);
        Serial.print(F("offset      = ")); Serial.println(*((long *)&data[1]), HEX);
        Serial.print(F("CRC32       = ")); Serial.println(*((long *)&data[2]), HEX);

        if (crc32_check((byte *)data, sizeof(data))) {
            Serial.println(F("CRC32 Pravilnaia."));
        } else {
            Serial.println(F("OSHIBKA CRC32."));
            Serial.print(F("Pravilnaiia CRC32: "));
            Serial.println(crc32_calc((byte *)data, sizeof(data) - sizeof(unsigned long)), HEX);
        }
        Serial.println();
    }
    break;

    // Сброс запомненных отказов.
    case '-':
    {
        Serial.println();
        loop_info.reset_loop(T0);
        Serial.println(F("Sbros otkazov vipolnen."));
        Serial.println();
    }
    break;

    // Масштаб тензодатчика += 100.
    case 'q':
    case 'Q':
    {
        Serial.println();
        Serial.println(F("Masshtab tensozodat4ika += 100."));
        Serial.print(F("Tekushiy scale = ")); Serial.println(loadcell.get_scale(), 5);
        Serial.print(F("Noviy scale    = ")); Serial.println(loadcell.set_scale(loadcell.get_scale() + 100.0f), 5);
        Serial.println();
    }
    break;

    // Масштаб тензодатчика -= 100.
    case 'a':
    case 'A':
    {
        Serial.println();
        Serial.println(F("Masshtab tensozodat4ika -= 100."));
        Serial.print(F("Tekushiy scale = ")); Serial.println(loadcell.get_scale(), 5);
        Serial.print(F("Noviy scale    = ")); Serial.println(loadcell.set_scale(loadcell.get_scale() - 100.0f), 5);
        Serial.println();
    }
    break;

    // Смещение тензодатчика += 0x3FF.
    case 'e':
    case 'E':
    {
        Serial.println();
        Serial.println(F("Smeshenie nulia += 0x3FF."));
        Serial.print(F("Tekushiy offset = ")); Serial.println(loadcell.get_offset(), HEX);
        Serial.print(F("Noviy offset    = ")); Serial.println(loadcell.set_offset(loadcell.get_offset() + 0x3FF), HEX);
        Serial.println();
    }
    break;

    // Смещение тензодатчика -= 0x3FF.
    case 'd':
    case 'D':
    {
        Serial.println();
        Serial.println(F("Smeshenie nulia -= 0x3FF."));
        Serial.print(F("Tekushiy offset = ")); Serial.println(loadcell.get_offset(), HEX);
        Serial.print(F("Noviy offset    = ")); Serial.println(loadcell.set_offset(loadcell.get_offset() - 0x3FF), HEX);
        Serial.println();
    }
    break;

    // Ручная установка масштаба тензодатчика, ввод вещественного числа.
    case 'f':
    case 'F':
    {
        Serial.println();
        Serial.println(F("Ru4naia ustanovka masshtaba tenzodat4ika."));
        Serial.print(F("Tekushiy scale: ")); Serial.println(loadcell.get_scale(), 5);
        Serial.print(F("Vvedite noviy scale: ")); loadcell.set_scale(l_scanf_double());
        Serial.print(F("Scale ustanovlen   : ")); Serial.println(loadcell.get_scale(), 5);
        Serial.println();
    }
    break;

    // Сохранение текущих настроек тензодатчика в EEPROM.
    case 's':
    case 'S':
    {
        Serial.println();
        Serial.println(F("Sphraniiaem v EEPROM: "));
        Serial.print(F("scale  = ")); Serial.println(loadcell.get_scale(), 5);
        Serial.print(F("offset = ")); Serial.println(loadcell.get_offset(), HEX);
        if (EEPROM_save(loadcell.get_scale(), loadcell.get_offset())) {
            Serial.println(F("Tekushie nastroyki sohraneni v EEPROM."));
        } else {
            Serial.println(F("OSHIBKA sohraneniia nastroek v EEPROM."));
        }
        Serial.println();
    }
    break;

    // Настройка тензодатчика.
    case 't':
    case 'T':
    {
        Serial.println();
        HX711_setup(loadcell);
        // Сбросим состояние выполнение циклов.
        loop_info.reset_loop(T0);
        Serial.println();
    }
    break;

    // Изменение настройки смещения нуля тензодатчика на основе текущих данных веса тензодатчика.
    case 'o':
    case 'O':
    {
        Serial.println();
        Serial.print(F("Predidushit offset: ")); Serial.println(loadcell.get_offset(), HEX);
        Serial.print(F("V masshtabe, kg   : ")); Serial.println(loadcell.get_units(), 5);
        Serial.print(F("Noviy offset      : ")); Serial.println(loadcell.set_offset(loadcell.tare()), HEX);
        Serial.print(F("V masshtabe, kg   : ")); Serial.println(loadcell.get_units(), 5);
        Serial.println();
    }
    break;

    // Кадр помощи: управление выводом, горячие клавиши.
    case 'h':
    case 'H':
    case ' ':
    default:
    {
        Serial.println();
        Serial.println(F("Upravlenie vivodom, goria4ie klavishi."));
        Serial.println(F("0       : Informaciia o rabote FADEC."));
        Serial.println(F("1       : Tekushie nastroyki tenzodat4ika."));
        Serial.println(F("-       : Sbros zapomnennih otkazov."));
        Serial.println(F("q/Q(+)  : Masshtab tensozodat4ika += 100."));
        Serial.println(F("a/A(-)  : Masshtab tensozodat4ika -= 100."));
        Serial.println(F("e/E(+)  : Smeshenie nulia += 0x3FF."));
        Serial.println(F("d/D(-)  : Smeshenie nulia -= 0x3FF."));
        Serial.println(F("f/F     : Ru4naia ustanovka masshtaba, vvod 4isla."));
        Serial.println(F("s/S     : Sohranenie uto4nennogo masshtaba, smesheniia."));
        Serial.println(F("t/T     : Nastroyka tenzodat4ika."));
        Serial.println(F("o/O     : Nastroyka smesheniia nulia tenzodat4ika."));
        Serial.println(F("h/SPACE : Upravlenie vivodom, goria4ie klavishi."));
        Serial.println();
    }
    break;

    }
}

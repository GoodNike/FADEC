﻿/**
 * @copyright   Николай Крашенинников
 * @project     Измерительный комплекс СУ
 * @file        crc32.h
 * @brief       Алгоритм подсчета CRC32, заголовочный файл.
 *
 * $Revision: 40 $
 * $Author: Николай $
 * $Date:: 2015-11-05 20:01:39#$
 */

#ifndef CRC32_H
#define CRC32_H 1

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

/**
 * Подсчет контольной суммы по массиву данных.
 * Подсчет происходит по внутреннему алгоритму, описание алгоритма недоступно.
 *
 * @param[in] arr Указатель на массив данных.
 * @param[in] sz Размер массива.
 * @return Подсчитанная CRC32.
 */
unsigned long crc32_calc(const byte *arr, size_t sz);

/**
 * Проверка контрольной суммы массива данных.
 * Функция подсчитывает контрольную сумму массива данных и сверяет ее с той, которая находится в конце
 * последовательности данных. Подсчет контрольной суммы происходит по внутреннему алгоритму, описание
 * алгоритма недоступно. Соответственно, сверка контрольной суммы пройдет успешно, если предварительно
 * она была сформирована #crc32_calc().
 *
 * @param[in] arr Указатель на массив данных.
 * @param[in] sz Размер массива.
 * @return Верная ли контрольная сумма: true верная, false ошибочная.
 *
 * @note Передаваемый массив должен в себе содержать и контрольную сумму для сверки. В передаваемом
 *       массиве сначала идет сам массив, а последнии четыре байта CRC32 этого массива.
 */
bool crc32_check(const byte *arr, size_t sz);

#endif  /* CRC32_H */

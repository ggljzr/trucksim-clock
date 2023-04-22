/**
 * @file pins.h
 *
 * @brief This file contains the pin definitions for the project and LaskaKit ESP32-C3-LPKit board.
 */

#pragma once

#include <stdint.h>

namespace esp32pins
{
    constexpr uint8_t kI2CSdaPin{8};
    constexpr uint8_t kI2CSclPin{10};
} // namespace esp32pins
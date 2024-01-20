/**
 * @file pins.h
 *
 * @brief This file contains the pin definitions for the project and LaskaKit ESP32-C3-LPKit board.
 */

#pragma once

#include <stdint.h>

namespace esp32pins
{
    // LaskaKit ESP32-C3-LPKit board pin definitions.
    // constexpr uint8_t kI2CSdaPin{8};
    // constexpr uint8_t kI2CSclPin{10};

    // Switch for turning device on and off (sleep mode).
    // constexpr uint8_t kOnOffSwitchPin{6};

    // LaskaKit ESP32 LPKit board pin definitions.
    constexpr uint8_t kI2CSdaPin{21};
    constexpr uint8_t kI2CSclPin{22};

    // Switch for turning device on and off (sleep mode).
    constexpr uint8_t kOnOffSwitchPin{32};

} // namespace esp32pins
/**
 * @file config.h
 *
 * @brief MQTT topic routes.
 */

#pragma once

namespace trucksim_topics
{
    auto constexpr kGameInfo = "trucksim/gameinfo";
    auto constexpr kTime = "trucksim/channel/game/time";
    auto constexpr kDistance = "trucksim/channel/truck/navigation/distance";
    auto constexpr kEta = "trucksim/channel/truck/navigation/time";
    auto constexpr kRestStop = "trucksim/channel/rest/stop";
}
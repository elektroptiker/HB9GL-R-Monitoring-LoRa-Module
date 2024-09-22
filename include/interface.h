#pragma once
#include <cstdint>

struct pc_link_message final
{
    constexpr static const uint32_t command = 1;
    uint8_t UplinkStatus;
    uint8_t EcholinkStatus;
};

struct esp_get_keepAlive_message final
{
    constexpr static const uint32_t command = 2;
    uint32_t dummy;
};

struct esp_get_message final
{
    constexpr static const uint32_t command = 3;
    uint32_t dummy;
};

struct esp_get_response_message final
{
    constexpr static const uint32_t command = 4;
    uint8_t aprsPacketSeq;
    float intvoltage;
    uint8_t battPercent;
    uint8_t MAINSpower;
    float temperature;
    float humidity;
    uint32_t lastAPRSDataTime;
    uint32_t lastAPRSStatusTime;
};

struct esp_get_reboot_message final
{
    constexpr static const uint32_t command = 5;
    uint32_t dummy;
};

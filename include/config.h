#pragma once

#include <cstdint>
#include <string>

// GENERAL SETTINGS

struct Basic_settings
{
    const std::string version{"1.0a"};
    const int EEPROMaddress{0};
};

// TELEMETRY SERVICE SETTINGS
struct Telemetry_settings
{
    const std::string destcall = "TLM";
    const std::uint8_t beacon{15};
    // const std::string callsign{"HB9HDG-13"};
    // const std::string lat{"4704.10N"};
    // const std::string lon{"00903.30E"};
    // const std::string alt{"001476"};     // 450m
    const std::string callsign{"HB9GL-15"}; // Passcode for HB9GL-R: 7370
    const std::string lat{"4704.16N"};      // 47°04'16.5"N 9°05'26.5"E
    const std::string lon{"00905.26E"};
    const std::string alt{"004557"}; // 1389m
    const std::string comment{"HB9GL-R 438.975 | Rx:-7.6 MHz | T:71.9Hz"};
    const bool use_status{true}; // send status below in timeout of igate packet (needs wifi, igate on and aprs-is)
    const std::string status{"https://www.hb9gl.ch"};
    const std::uint8_t hall_sensor_pin{35};
    const std::uint8_t green_led_pin{25};
    const std::uint8_t usb_power_pin{34};
    const std::uint8_t mains_power_pin{36}; // must not use pin 35 on 8MB modules!
    const std::uint8_t dht11_pin{0};
};


// LORA MODULE SETTINGS (keep default unless experimental setup)
struct LoRa_settings
{
    const unsigned long serial_baud{115200L};
    const std::uint32_t frequency{433775000U}; // Frequency (433775000 Hz)
    const std::int16_t TxPower{20};            // TX power (max 20 dBm)
    const std::int16_t SCK_pin{5};
    const std::int16_t MISO_pin{19};
    const std::int16_t MOSI_pin{27};
    const std::int16_t SS_pin{18};
    const std::int16_t RST_pin{14};
    const std::int16_t DIO0_pin{26};
    const std::int16_t SpreadingFactor{12};
    const unsigned long SignalBandwidth{125000L};
    const std::int16_t CodingRate4{5};
};

struct Settings
{
    const Basic_settings basic;
    const Telemetry_settings tlm;
    const LoRa_settings lora;
};

// STRINGS
struct Texts
{
    const std::string app_title = "HB9HDGs LinkObs";
    const std::string app_copyright{"(c)2024 Daniel G. HB9HDG"};
    const std::string on{"on"};
    const std::string off{"off"};
    const std::string battery{"Vbattery = %2.1fV (%d%%)"};
    const std::string temp_hum{"Temp: %.1f°C | Humid: %-.0f%%"};
    const std::string usb_pwr{"USB-Pwr"};
    const std::string ext_pwr{"Ext-Pwr"};
    const std::string pc_conn{"PC"};
    const std::string net_uplink{"Uplink"};
    const std::string net_echolink{"Echolink"};
};

#pragma once

#include <cstdint>
#include <string>

// general settings file


struct Settings
{
    // basic settings
    struct Basic_settings
    {
        const std::string version{"1.1"};
        const int EEPROMaddress{0};
        const unsigned long serial_baud{115200L};
        const uint8_t display_address{0x3c};
        const int display_sda{21};
        const int display_scl{22};
        const std::uint8_t green_led_pin{25}; // digital output
    } basic;

    // TELEMETRY SERVICE SETTINGS
    struct Telemetry_settings
    {
        const std::string callsign{"HB9HDG-13"};
        const std::string lat{"4704.10N"};
        const std::string lon{"00903.30E"};
        const std::string alt{"001476"};                        // 450m
        const std::string comment{"HB9HDG aprs telemetry 1.1"}; //  max 43 chars!
        // const std::string callsign{"HB9GL-15"}; // Passcode for HB9GL-R: 7370
        // const std::string lat{"4704.16N"};      // 47°04'16.5"N 9°05'26.5"E
        // const std::string lon{"00905.26E"};
        // const std::string alt{"004557"};                                          // 1389m
        // const std::string comment{"HB9GL-R 438.975, -7.6 | T:71.9 | Node:41140"}; //  max 43 chars!
        const std::string destcall = "TLM";
        const unsigned long beacon_interval{15}; // time [min] between beacons
        const unsigned long status_interval{60}; // time [min] betwenn status packet transmitting
        const std::uint8_t hall_sensor_pin{35};  // battery voltage
        const std::uint8_t usb_power_pin{34};    // digital input
        const unsigned long pc_timeout{30};      // time [sec] until pc gets status unreachable
        const std::uint8_t ext_power_pin{36};    // digital input
        const std::uint8_t dht11_pin{0};
        const unsigned long dht11_interval{30}; // time [sec] between dht11 readings
    } tlm;
    // const LoRa_settings lora;
    struct LoRa_settings
    // LORA MODULE SETTINGS (keep default unless experimental setup)
    {
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
    } lora;
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

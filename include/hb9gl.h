#pragma once

#include <DHT.h> // dht11 sensor (temperature & humidity)
#include <EEPROM.h>
#include <SSD1306.h> // LCD display
#include <Wire.h>
#include <config.h> // our configuration file
#include <cstdint>
#include <string>

class Data
{
public:
    Data(const DHT &dht) : m_dht(dht)
    {
#if SERIALDEBUG
        Serial.println("Data constructor called");
#endif
    };
    ~Data() = default;

    void init();
    void updateData();
    void fetchSensorData();
    float get_temperature();
    float get_humidity();
    float get_intVoltage();
    int get_battPercent();
    uint8_t get_aprsPacketSeq();
    void set_aprsPacketSeq(uint8_t count);
    void inc_aprsPacketSeq();
    bool get_statusPCUSBpower();
    bool get_statusMainsPower();
    bool get_statusPCConnected();
    void set_statusPCConnected(bool pcconnected);
    bool get_statusUpLink();
    void set_statusUpLink(bool uplink_reachable);
    bool get_statusEchoLink();
    void set_statusEchoLink(bool echolink_logged_in);
    bool get_statusChanged();
    void reset_statusChanged();

protected:
    const Settings settings;
    // const Basic_settings m_basicSettings;
    // const Telemetry_settings m_tlmSettings;
    // const LoRa_settings m_loraSettings;
    const Texts m_txt;
    float m_temperature{};
    float m_humidity{};
    float m_intvoltage{};
    uint8_t m_battPercent{};
    uint8_t m_aprsPacketSeq{0};
    bool m_statusPCconnected{false};
    bool m_previousStatusPCconnected{false};
    bool m_statusPCUSBpower{false};
    bool m_previousStatusUSBPower{false};
    bool m_statusMainsPower{false};
    bool m_previousStatusMainsPower{false};
    bool m_statusUpLink{false};
    bool m_previousStatusUpLink{false};
    bool m_statusEchoLink{false};
    bool m_previousStatusEchoLink{false};

private:
    const unsigned long m_battWaitTime{15000};
    unsigned long m_battTimeStamp{0};
    DHT m_dht;
    const unsigned long m_dhtWaitTime{15000};
    unsigned long m_dhtTimeStamp{0};
};

class Display : public Data
{
public:
    Display(SSD1306 &lcd, DHT &dht) : Data(dht), m_lcd(lcd)
    {
#if SERIALDEBUG
        Serial.println("Display constructor called");
#endif
    };
    ~Display() {};

    void init();
    void displayData();
    void printBox(int16_t x, int16_t y, int16_t width, int16_t height, const String &text, bool inverse = false);

private:
    SSD1306 m_lcd;
};

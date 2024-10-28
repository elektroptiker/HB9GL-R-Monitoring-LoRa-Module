#include <Arduino.h>
#include <config.h>    // our configuration file
#include <hb9gl.h>     // data and display handling
#include <interface.h> // USB communication definition with PC-Compagnion
#include <mylora.h>    // lora handling

// defines for debugging purpuoses
#define LORA true         // enable LoRa tx
#define SERIALDEBUG false // use usb/serial for debug instead communication with PC-Compagnion
#define SERIALDATA !SERIALDEBUG

const Settings settings;

EspClass esp;
DHT dht;
SSD1306 lcd(settings.basic.display_address, settings.basic.display_sda, settings.basic.display_scl);
Display display(lcd, dht);
MyLora lora;

static time_t aprsLastReconnect = 0;
static time_t lastIgBeacon = 0;
static time_t lastMtBeacon = 0;
static time_t lastUpload = 0;

unsigned long lastSerialPacketReceived = 0;
unsigned long KeepAliveInterval = settings.tlm.pc_timeout * 1000L;

// basic timer struct
struct TIMER
{
    unsigned long stamp;
    unsigned long duration;
    bool state;
};

TIMER statusBlink{0, 1000, 0};                                         // timer to control LED blinking with 1000 msec
TIMER tmrAPRSsendData{0, settings.tlm.beacon_interval * 60 * 1000, 0}; // timer to tx APRS Data
TIMER tmrAPRSsendStatus{0, 1 * settings.tlm.status_interval * 60 * 1000, 0}; // timer to tx APRS Status
TIMER tmrGetDHT11Data{0,
                      settings.tlm.dht11_interval * 1000,
                      0}; // timer to get new environmental data from DHT11 sensor

unsigned long currentTime;

void setup()
{
    Serial.begin(settings.basic.serial_baud);
    display.init();

#if SERIALDEBUG
    Serial.print("\n\nLoRa telemetry v");
    Serial.print(settings.basic.version.c_str());
    Serial.println("\nby HB9HDG\n");
#endif
    pinMode(settings.basic.green_led_pin, OUTPUT);
    pinMode(settings.tlm.usb_power_pin, INPUT);
    pinMode(settings.tlm.ext_power_pin, INPUT);

#if SERIALDEBUG
    Serial.println("{setup} LoRa Setup init");
#endif
    lora.init();

#if SERIALDEBUG
    Serial.println("{setup} Display data");
#endif
    display.updateData();
    display.displayData();

#if SERIALDEBUG
    Serial.println("{setup} tx_telemetry_beacon");
#endif
    tmrAPRSsendStatus.stamp = millis();
    lora.tx_telemetry_beacon(display);
#if SERIALDEBUG
    Serial.println("{setup} tx_telemetry_data");
#endif
    tmrAPRSsendData.stamp = millis();
    lora.tx_telemetry_data(display);
#if SERIALDEBUG
    Serial.println("{setup} Startup finished.");
#endif
    display.reset_statusChanged();
}

void loop()
{
    // auto restart in case something unexpected happens
    if (millis() > 23L * 59L * 60L * 1000L)
        esp.restart();

    currentTime = millis();

    // blink status LED
    if (currentTime - statusBlink.stamp >= statusBlink.duration)
    {
        statusBlink.stamp = currentTime;
        statusBlink.state = !statusBlink.state;
        digitalWrite(settings.basic.green_led_pin, statusBlink.state);
    }

#if SERIALDATA
    // serial communication with pc-compagnion
    // look for incoming serial packets
    while (Serial.available() >= sizeof(uint32_t))
    {
        lastSerialPacketReceived = currentTime;
        display.set_statusPCConnected(true);
        auto cmd = uint32_t{};
        // read the command
        if (sizeof(cmd) == Serial.read(( uint8_t * )&cmd, sizeof(cmd)))
        {
            // read the appropriate message and
            // update the system accordingly
            switch (cmd)
            {
            case pc_link_message::command:
            {
                pc_link_message msg;
                Serial.read(( uint8_t * )&msg, sizeof(msg));
                display.set_statusUpLink(msg.UplinkStatus);
                display.set_statusEchoLink(msg.EcholinkStatus);
            }
            break;
            case esp_get_keepAlive_message::command:
            {
                esp_get_keepAlive_message msg;
                Serial.read(( uint8_t * )&msg, sizeof(msg));
            }
            break;
            case esp_get_message::command:
            {
                esp_get_message msg;
                Serial.read(( uint8_t * )&msg, sizeof(msg));
                esp_get_response_message rsp;
                display.updateData();
                rsp.aprsPacketSeq = display.get_aprsPacketSeq();
                rsp.intvoltage = display.get_intVoltage();
                rsp.battPercent = display.get_battPercent();
                rsp.MAINSpower = display.get_statusMainsPower() ? 1 : 0;
                rsp.temperature = display.get_temperature();
                rsp.humidity = display.get_humidity();
                rsp.lastAPRSDataTime = (millis() - tmrAPRSsendData.stamp) / 1000;
                rsp.lastAPRSStatusTime = (millis() - tmrAPRSsendStatus.stamp) / 1000;
                cmd = esp_get_response_message::command;
                while (!Serial.availableForWrite())
                {
                    delay(1);
                }
                Serial.write(( uint8_t * )&cmd, sizeof(cmd));
                while (!Serial.availableForWrite())
                {
                    delay(1);
                }
                Serial.write(( uint8_t * )&rsp, sizeof(rsp));
            }
            break;
            case esp_get_reboot_message::command:
            {
                esp_get_reboot_message msg;
                Serial.read(( uint8_t * )&msg, sizeof(msg));
                esp.restart();
            }
            break;
            default:
                // junk. consume what we didn't read
                while (Serial.available())
                    Serial.read();
                break;
            }
        }
        else
        {
            // junk. consume what we didn't read
            while (Serial.available())
                Serial.read();
        }
    }
#endif

    if (currentTime - lastSerialPacketReceived >= KeepAliveInterval)
    {
        display.set_statusPCConnected(false);
        display.set_statusUpLink(false);
        display.set_statusEchoLink(false);
    }
    display.updateData();

    // send aprs status messages (position and tlm-parameters)
    if (currentTime - tmrAPRSsendStatus.stamp >= tmrAPRSsendStatus.duration)
    {
#if SERIALDEBUG
        Serial.println("{loop} aprs status timer reached.");
#endif
        tmrAPRSsendStatus.stamp = currentTime;
        lora.tx_telemetry_beacon(display);
    }
    // send aprs telemetry data
    if (currentTime - tmrAPRSsendData.stamp >= tmrAPRSsendData.duration)
    {
#if SERIALDEBUG
        Serial.println("{loop} aprs telemetry timer reached.");
#endif
        tmrAPRSsendData.stamp = currentTime;
        lora.tx_telemetry_data(display);
    }

    if (display.get_statusChanged())
    {
        display.reset_statusChanged();
        display.displayData();
        tmrAPRSsendStatus.stamp = currentTime;
        lora.tx_telemetry_data(display);
    }
    if (currentTime % 50)
    {
        display.displayData();
    }
}

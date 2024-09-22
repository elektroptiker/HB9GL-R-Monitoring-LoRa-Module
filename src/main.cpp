#include <Arduino.h>
#include <LoRa.h>
#include <Wire.h>
#include <config.h>
#include <hb9gl.h>
#include <interface.h>
#include <string>

#define LORA true
#define SERIALDEBUG false
#define SERIALDATA !SERIALDEBUG

EspClass esp;
DHT dht;
SSD1306 lcd(0x3c, 21, 22);
Display display(lcd, dht);

const Settings settings;

void lora_setup();
void lora_send(String tx_data);
template <typename T>
void lora_send(T tx_data);
void beacon_telemetry();
void beacon_telemetry_DATA();

static time_t aprsLastReconnect = 0;
static time_t lastIgBeacon = 0;
static time_t lastMtBeacon = 0;
static time_t lastUpload = 0;

// last time we connected to the wu server in milliseconds
// unsigned long lastAPRSDataTime = 0;
// unsigned long lastAPRSStatusTime = 0;
unsigned long lastSerialPacketReceived = 0;
unsigned long KeepAliveInterval = 30L * 1000L; // after 30seconds we assume the PC offline
struct TIMER
{
    unsigned long stamp;
    unsigned long duration;
    bool state;
};
// TIMER ledBlink {0, 100, 0};  // timer for LED to blink with 100 msec
// TIMER blinkBlink {0, 2000, 0}; // timer to control LED blinking with 2000 msec
TIMER statusBlink{0, 1000, 0};                     // timer to control LED blinking with 1000 msec
TIMER tmrAPRSsendData{0, 15 * 60 * 1000, 0};       // timer to send APRS Data every 15 minutes
TIMER tmrAPRSsendStatus{0, 1 * 60 * 60 * 1000, 0}; // timer to send APRS Status every hour
TIMER tmrGetDHT11Data{0, 30 * 1000, 0};            // timer to get new environment data from DHT11 sensor
TIMER tmrToggleMains{0, 5000, 0};                  // timer for testing Mains power toggle every 5 seconds
unsigned long currentTime;

/**
 * @brief inserts leading "0" to the string up to the specified lenght
 */
String padzero(String &number, byte length)
{
    while (number.length() < length)
        number = "0" + number;
    return number;
}

/**
 * @brief inserts leading "0" to the string up to the specified lenght
 */
template <typename T>
String padzero(T &number, byte length)
{
    auto tmpStr = String(number);
    while (tmpStr.length() < length)
        tmpStr = "0" + tmpStr;
    return tmpStr;
}

void setup()
{
    Serial.begin(settings.lora.serial_baud);
    display.init();

#if SERIALDEBUG
    Serial.print("\n\nLoRa telemetry v");
    Serial.print(settings.basic.version.c_str());
    Serial.println("\nby HB9HDG\n");
#endif
    pinMode(settings.tlm.green_led_pin, OUTPUT);
    pinMode(settings.tlm.usb_power_pin, INPUT);
    pinMode(settings.tlm.mains_power_pin, INPUT);
#if SERIALDEBUG
    Serial.println("{setup} LoRa Setup init");
#endif

    lora_setup();

#if SERIALDEBUG
    Serial.println("{setup} Display data");
#endif
    display.updateData();
    display.displayData();

#if SERIALDEBUG
    Serial.println("{setup} beacon_telemetry");
#endif
    beacon_telemetry();
#if SERIALDEBUG
    Serial.println("{setup} beacon_telemetry_DATA");
#endif
    beacon_telemetry_DATA();
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
        digitalWrite(settings.tlm.green_led_pin, statusBlink.state);
    }

#if SERIALDATA
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

    // send aprs status messages
    if (currentTime - tmrAPRSsendStatus.stamp >= tmrAPRSsendStatus.duration)
    {
#if SERIALDEBUG
        Serial.println("{loop} aprs status timer reached.");
#endif
        tmrAPRSsendStatus.stamp = currentTime;
        beacon_telemetry();
    }
    // send aprs telemetry data
    if (currentTime - tmrAPRSsendData.stamp >= tmrAPRSsendData.duration)
    {
#if SERIALDEBUG
        Serial.println("{loop} aprs telemetry timer reached.");
#endif
        tmrAPRSsendData.stamp = currentTime;
        beacon_telemetry_DATA();
    }

    if (display.get_statusChanged())
    {
        display.reset_statusChanged();
        display.displayData();
        beacon_telemetry_DATA();
    }
    if (currentTime % 50)
    {
        display.displayData();
    }
}

void lora_setup()
{
    SPI.begin(settings.lora.SCK_pin, settings.lora.MISO_pin, settings.lora.MOSI_pin, settings.lora.SS_pin);
    LoRa.setPins(settings.lora.SS_pin, settings.lora.RST_pin, settings.lora.DIO0_pin);
    if (!LoRa.begin(settings.lora.frequency))
    {
        // Serial.println("Failed to setup LoRa module.");
        while (1)
            ;
    }
    LoRa.setSpreadingFactor(settings.lora.SpreadingFactor);
    LoRa.setSignalBandwidth(settings.lora.SignalBandwidth);
    LoRa.setCodingRate4(settings.lora.CodingRate4);
    LoRa.enableCrc();
    LoRa.setTxPower(settings.lora.TxPower);
    delay(3000);
    LoRa.sleep();
}

void lora_send(String tx_data)
{
#if SERIALDEBUG
    Serial.println("Function called: lora_send");
#endif
    digitalWrite(settings.tlm.green_led_pin, HIGH);
#if LORA
    LoRa.setFrequency(settings.lora.frequency);
    LoRa.beginPacket();
    LoRa.write('<');
    LoRa.write(0xFF);
    LoRa.write(0x01);
#if SERIALDEBUG
    Serial.println("TX: " + tx_data);
#endif
    LoRa.write(( const uint8_t * )tx_data.c_str(), tx_data.length());
    LoRa.endPacket();
    LoRa.setFrequency(settings.lora.frequency);
    LoRa.sleep();
#endif
    digitalWrite(settings.tlm.green_led_pin, LOW);
}

template <typename T>
void lora_send(T tx_data)
{
#if SERIALDEBUG
    Serial.println("Function called: lora_send");
#endif

    digitalWrite(settings.tlm.green_led_pin, HIGH);
#if LORA
    LoRa.setFrequency(settings.lora.frequency);
    LoRa.beginPacket();
    LoRa.write('<');
    LoRa.write(0xFF);
    LoRa.write(0x01);
#if SERIALDEBUG
    Serial.println("TX: " + tx_data);
#endif
    LoRa.write(( const uint8_t * )tx_data.c_str(), tx_data.length());
    LoRa.endPacket();
    LoRa.setFrequency(settings.lora.frequency);
    LoRa.sleep();
#endif
    digitalWrite(settings.tlm.green_led_pin, LOW);
}

void beacon_telemetry()
{
#if SERIALDEBUG
    Serial.println("Function called: beacon_telemetry");
#endif

    tmrAPRSsendStatus.stamp = millis();
#if LORA

    // String Beacon = String(TELEMETRY_CALLSIGN) + ">" + String(DESTCALL_TELEMETRY) + ":=" + String(TELEMETRY_LAT) + "/" + String(TELEMETRY_LON) + "_.../" + String(TELEMETRY_COMMENT);
    String Beacon = String(settings.tlm.callsign.c_str()) + ">" + String(settings.tlm.destcall.c_str()) + ":!" +
                    String(settings.tlm.lat.c_str()) + "/" + String(settings.tlm.lon.c_str()) + "r" +
                    String(settings.tlm.comment.c_str()) + "/A=" + String(settings.tlm.alt.c_str());
    lora_send(Beacon);

    // Example:
    // :N0QBF-11V:PARM.Battery,Btemp,ATemp,Pres,Alt,Camra,Chut,Sun,10m,ATV
    Beacon = String(settings.tlm.callsign.c_str()) + ">" + String(settings.tlm.destcall.c_str()) +
             "::" + String(settings.tlm.callsign.c_str()) +
             ":PARM.Vbatt,Capacity,Temperature,Humidity,,USBPower,240V,PCconn,Uplink,Echolink";
    lora_send(Beacon);

    // Example
    // :N0QBF-11V:UNIT.v/100,deg.F,deg.F,Mbar,Kft,Click,OPEN,on,on,hi
    Beacon = String(settings.tlm.callsign.c_str()) + ">" + String(settings.tlm.destcall.c_str()) +
             "::" + String(settings.tlm.callsign.c_str()) + ":UNIT.Vdc,%,Celsius,%,,UP,UP,UP,UP,UP";
    lora_send(Beacon);

    // Example
    // :N0QBF-11V:EQNS.0,5.2,0,0,.53,-32,3,4.39,49,-32,3,18,1,2,3
    Beacon = String(settings.tlm.callsign.c_str()) + ">" + String(settings.tlm.destcall.c_str()) +
             "::" + String(settings.tlm.callsign.c_str()) + ":EQNS.0,0.01,2.5,0,1,0,0,1,-100,0,1,0";
    lora_send(Beacon);

    // Example
    // :N0QBF-11V:BITS.10110000,N0QBF’s Big Balloon
    Beacon = String(settings.tlm.callsign.c_str()) + ">" + String(settings.tlm.destcall.c_str()) +
             "::" + String(settings.tlm.callsign.c_str()) + ":BITS.HB9GL-R telemetry by HB9HDG";
    lora_send(Beacon);
#endif
}

/*

https://w4krl.com/sending-aprs-analog-telemetry-the-basics/

Cpu: 0.270 Load (TLM: 270 EQN: 0,0.001,0)
Temp: 37.552 DegC (TLM: 37552 EQN: 0,0.001,0)
FreeM: 805 Mb (TLM: 805 EQN: 0,1,0)
RxP: 44 Pkt (TLM: 44 EQN: 0,1,0)
TxP: 25 Pkt (TLM: 25 EQN: 0,1,0)


VE6AGD-11>APRS,WIDE1-1,WIDE2-1,qAR,CALGRY::VE6AGD-11:PARAM.SlrPnl,Batt,Temp1,Temp2,Pres,blank,blank,blank,blank,Pos,BME280:Rate,Pwr
VE6AGD-11>APRS,WIDE1-1,WIDE2-1,qAR,CALGRY::VE6AGD-11:UNIT.v/100,v/100,degF,degF,Mbar,0,0,0,0,true,on,slow,high
VE6AGD-11>APRS,WIDE1-1,WIDE2-1,qAR,CALGRY::VE6AGD-11:EQNS.0,0.01,0,0,0.01,0,0,1,0,0,1,0,0,1,0
VE6AGD-11>APRS,WIDE1-1,WIDE2-1,qAR,CALGRY::VE6AGD-11:BITS.00000111,VE6AGD-11 Balloon
VE6AGD-11>APRS,WIDE1-1,WIDE2-1,qAR,CALGRY:T#003,506,423,74,-196,0,00001001,1284R823_1284L34

Projekt:	VE6AGD-11 Balloon
Werte:	Solar: 4.850 volts (TLM: 485 EQN: 0,0.01,0)
Batt: 4.190 volts (TLM: 419 EQN: 0,0.01,0)
Intemp: 69 degF (TLM: 69 EQN: 0,1,0)
Extemp: 67 degF (TLM: 67 EQN: 0,1,0)
Pres: 892 Mbar (TLM: 892 EQN: 0,1,0)
Bit-Bedeutung:	 GPS     BME     Per     Pwr    (BITS: 00011111)
*/
void beacon_telemetry_DATA()
{
#if SERIALDEBUG
    Serial.println("Function called: beacon_telemetry_DATA");
#endif

    tmrAPRSsendData.stamp = millis();

    display.inc_aprsPacketSeq();

    //write sequence to eeprom
    EEPROM.write(settings.basic.EEPROMaddress, display.get_aprsPacketSeq());
    EEPROM.commit();

    byte txvoltage = (display.get_intVoltage() - 2.5) / (2.5 / 255);
    byte txtemperature = (display.get_temperature() + 100);
    byte txhumidity = display.get_humidity();
    auto txaprsPacketSeq = display.get_aprsPacketSeq();
    auto txbattPercent = display.get_battPercent();
    String txbits = "00000";

    if (display.get_statusPCUSBpower())
        txbits.setCharAt(0, '1');
    if (display.get_statusMainsPower())
        txbits.setCharAt(1, '1');
    if (display.get_statusPCConnected())
        txbits.setCharAt(2, '1');
    if (display.get_statusUpLink())
        txbits.setCharAt(3, '1');
    if (display.get_statusEchoLink())
        txbits.setCharAt(4, '1');

    String Beacon = String(settings.tlm.callsign.c_str()) + ">" + String(settings.tlm.destcall.c_str()) + ":T#" +
                    padzero(txaprsPacketSeq, 3) + "," + padzero(txvoltage, 3) + "," + padzero(txbattPercent, 3) + "," +
                    padzero(txtemperature, 3) + "," + padzero(txhumidity, 3) + ",," + txbits;
#if SERIALDEBUG
    Serial.print("beacon_telemetry_DATA beacon:");
    Serial.println(Beacon);
#endif

#if LORA
    lora_send(Beacon);
#endif
}

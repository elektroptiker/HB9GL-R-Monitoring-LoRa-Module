#include <Arduino.h>
#include <Wire.h>
#include <hb9gl.h>
#include <mylora.h>

#define LORA true         // enable LoRa tx
#define SERIALDEBUG false // use usb/serial for debug instead communication with PC-Compagnion
#define SERIALDATA !SERIALDEBUG


/**
 * @brief initializes the LoRa radio module
 * @note code mostly from library examples
 *
 */
void MyLora::init()
{
    SPI.begin(m_settings.lora.SCK_pin, m_settings.lora.MISO_pin, m_settings.lora.MOSI_pin, m_settings.lora.SS_pin);
    setPins(m_settings.lora.SS_pin, m_settings.lora.RST_pin, m_settings.lora.DIO0_pin);
    if (!begin(m_settings.lora.frequency))
    {
#if SERIALDEBUG
        Serial.println("Failed to setup LoRa module.");
#endif
        while (1)
            ;
    }
    setSpreadingFactor(m_settings.lora.SpreadingFactor);
    setSignalBandwidth(m_settings.lora.SignalBandwidth);
    setCodingRate4(m_settings.lora.CodingRate4);
    enableCrc();
    setTxPower(m_settings.lora.TxPower);
    delay(3000);
    sleep();
}


/**
 * @brief transmit aprs string
 *
 * @param tx_data string to tx
 */
void MyLora::tx(String tx_data)
{
#if SERIALDEBUG
    Serial.println("Function called: tx_lora");
#endif
    digitalWrite(m_settings.basic.green_led_pin, HIGH);
#if LORA
    setFrequency(m_settings.lora.frequency);
    beginPacket();
    write('<');
    write(0xFF);
    write(0x01);
#if SERIALDEBUG
    Serial.println("TX: " + tx_data);
#endif
    write(( const uint8_t * )tx_data.c_str(), tx_data.length());
    endPacket();
    setFrequency(m_settings.lora.frequency);
    sleep();
#endif
    digitalWrite(m_settings.basic.green_led_pin, LOW);
}


/**
 * @brief transmit aprs data
 *
 * @tparam T
 * @param tx_data data to tx
 */
template <typename T>
void MyLora::tx(T tx_data)
{
#if SERIALDEBUG
    Serial.println("Function called: tx_lora");
#endif

    digitalWrite(m_settings.basic.green_led_pin, HIGH);
#if LORA
    setFrequency(m_settings.lora.frequency);
    beginPacket();
    write('<');
    write(0xFF);
    write(0x01);
#if SERIALDEBUG
    Serial.println("TX: " + tx_data);
#endif
    write(( const uint8_t * )tx_data.c_str(), tx_data.length());
    endPacket();
    setFrequency(m_settings.lora.frequency);
    sleep();
#endif
    digitalWrite(m_settings.basic.green_led_pin, LOW);
}


/**
 * @brief transmit APRS position and telemetry status and config
 */
void MyLora::tx_telemetry_beacon(Display &display)
{
#if SERIALDEBUG
    Serial.println("Function called: tx_telemetry_beacon");
#endif

#if LORA

    // send the status of HB9GL (root)
    // String beacon = String("HB9GL-0") + ">" + String(m_settings.tlm.destcall.c_str()) + ":!" +
    //                 String(m_settings.tlm.lat.c_str()) + "/" + String(m_settings.tlm.lon.c_str()) + "r" +
    //                 String(m_settings.tlm.comment.c_str()).substring(0, 43) + "/A=" + String(m_settings.tlm.alt.c_str());
    // tx_lora(beacon);

    // send status of current station
    String beacon = String(m_settings.tlm.callsign.c_str()) + ">" + String(m_settings.tlm.destcall.c_str()) + ":!" +
                    String(m_settings.tlm.lat.c_str()) + "/" + String(m_settings.tlm.lon.c_str()) + "r" +
                    String(m_settings.tlm.comment.c_str()).substring(0, 43) +
                    "/A=" + String(m_settings.tlm.alt.c_str());
    tx(beacon);

    /**
     * @brief set APRS telemetry parameters/titles
     * @note the 6 analog channels
     * @note then up to 8 digital channels
     */
    beacon = String(m_settings.tlm.callsign.c_str()) + ">" + String(m_settings.tlm.destcall.c_str()) +
             "::" + rpad(m_settings.tlm.callsign, 9).c_str() +
             ":PARM.Vbatt,Capacity,Temperature,Humidity,,USBPower,240V,PCconn,Uplink,Echolink";
    tx(beacon);

    /**
     * @brief set APRS telemetry parameters/units
     * @note first the 6 analog channels
     * @note then up to 8 digital channels
     */
    beacon = String(m_settings.tlm.callsign.c_str()) + ">" + String(m_settings.tlm.destcall.c_str()) +
             "::" + rpad(m_settings.tlm.callsign, 9).c_str() + ":UNIT.Vdc,%,Celsius,%,,UP,UP,UP,UP,UP";
    tx(beacon);

    /**
     * @brief set APRS telemetry parameters/equations for the 6 analog channels
     */
    beacon = String(m_settings.tlm.callsign.c_str()) + ">" + String(m_settings.tlm.destcall.c_str()) +
             "::" + rpad(m_settings.tlm.callsign, 9).c_str() + ":EQNS.0,0.01,2.5,0,1,0,0,1,-100,0,1,0";
    tx(beacon);

    beacon = String(m_settings.tlm.callsign.c_str()) + ">" + String(m_settings.tlm.destcall.c_str()) +
             "::" + rpad(m_settings.tlm.callsign, 9).c_str() + ":BITS.HB9GL-R telemetry by HB9HDG";
    tx(beacon);
#endif
}


/**
 * @brief send APRS telemetry data
 *
 */
void MyLora::tx_telemetry_data(Display &display)
{
#if SERIALDEBUG
    Serial.println("Function called: tx_telemetry_data");
#endif


    display.inc_aprsPacketSeq();

    //write aprs sequence to eeprom
    EEPROM.write(m_settings.basic.EEPROMaddress, display.get_aprsPacketSeq());
    EEPROM.commit();

    auto txvoltage = std::to_string((display.get_intVoltage() - 2.5) / (2.5 / 255));
    auto txtemperature = std::to_string((display.get_temperature() + 100));
    auto txhumidity = std::to_string(display.get_humidity());
    auto txaprsPacketSeq = std::to_string(display.get_aprsPacketSeq());
    auto txbattPercent = std::to_string(display.get_battPercent());

    std::string txbits = "00000"; // bits for each of the digital telemetry channels
    txbits[0] = display.get_statusPCUSBpower() ? '1' : '0';
    txbits[1] = display.get_statusMainsPower() ? '1' : '0';
    txbits[2] = display.get_statusPCConnected() ? '1' : '0';
    txbits[3] = display.get_statusUpLink() ? '1' : '0';
    txbits[4] = display.get_statusEchoLink() ? '1' : '0';

    auto beacon = m_settings.tlm.callsign + ">" + m_settings.tlm.destcall + ":T#" + lpad(txaprsPacketSeq, 3, '0') +
                  "," + lpad(txvoltage, 3) + "," + lpad(txbattPercent, 3) + "," + lpad(txtemperature, 3) + "," +
                  lpad(txhumidity, 3) + ",," + txbits;
#if SERIALDEBUG
    Serial.print("tx_telemetry_data beacon:");
    Serial.println(beacon);
#endif

#if LORA
    tx(beacon);
#endif
}

/**
 * @brief adds leading characters to a string
 *
 * @param str input string
 * @param s final length
 * @param paddedChar padded character (default: space)
 * @return std::string
 */
std::string MyLora::lpad(std::string const &str, size_t length, char paddedChar)
{
    if (str.size() < length)
    {
        return std::string(length - str.size(), paddedChar) + str;
    }
    else
    {
        return str;
    }
}

/**
 * @brief adds trailing characters to a string
 *
 * @param str input string
 * @param s final length
 * @param paddedChar padded character (default: space)
 * @return std::string
 */
std::string MyLora::rpad(std::string const &str, size_t s, char paddedChar)
{
    if (str.size() < s)
        return str + std::string(s - str.size(), paddedChar);
    else
        return str;
}

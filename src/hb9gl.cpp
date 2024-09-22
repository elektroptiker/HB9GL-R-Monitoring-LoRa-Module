#include <hb9gl.h>
#define SERIALDEBUG false
#define SERIALDATA !SERIALDEBUG


void Data::init()
{
#if SERIALDEBUG
    Serial.println("Data::Init");
#endif

    // read aprs sequence counter from eeprom
    EEPROM.begin(512);
    m_aprsPacketSeq = EEPROM.read(m_basicSettings.EEPROMaddress);
    if (isnan(m_aprsPacketSeq))
        m_aprsPacketSeq = 0;
        // if (m_aprsPacketSeq < 146)
        //     m_aprsPacketSeq = 146;
#if SERIALDEBUG
    Serial.println("EEPROM initiated");
    Serial.print("stored last APRS packet sequence#: ");
    Serial.println(m_aprsPacketSeq);
#endif


    // read internal battery status
    m_intvoltage = float(analogRead(m_tlmSettings.hall_sensor_pin)) / 4095 * 2 * 3.3 * 1.1;
    m_battPercent = 100 * (m_intvoltage - 3.3) / (4.2 - 3.3);
    if (m_battPercent > 100)
        m_battPercent = 100;
    if (m_battPercent < 0)
        m_battPercent = 0;
#if SERIALDEBUG
    Serial.print("internal battery percent: ");
    Serial.println(m_battPercent);
#endif

    // read dht22 sensor
    m_dht.setup(m_tlmSettings.dht11_pin);
    delay(1000);
    auto currentTime = millis();
    m_dhtTimeStamp = currentTime;
    m_temperature = m_dht.getTemperature();
    m_humidity = m_dht.getHumidity();
    if (isnan(m_temperature))
        m_temperature = 0.0f;
    if (isnan(m_humidity))
        m_humidity = 0.0f;

#if SERIALDEBUG
    Serial.println("DHT initiated");
    Serial.print("Temp: ");
    Serial.print(m_temperature);
    Serial.print("°C Humidity: ");
    Serial.print(m_humidity);
    Serial.println("%");
#endif
}

void Data::fetchSensorData()
{
#if SERIALDEBUG
    // Serial.println("{Data::fetchSensorData}");
#endif
    get_intVoltage();
    get_temperature();
    get_statusMainsPower();
    get_statusPCUSBpower();
}

void Data::updateData()
{
    fetchSensorData();
}

float Data::get_temperature()
{
    // read dht sensor data
    const auto currentTime = millis();
    if (currentTime - m_dhtTimeStamp >= m_dhtWaitTime)
    {
        m_dhtTimeStamp = currentTime;
        m_temperature = m_dht.getTemperature();
        if (isnan(m_temperature))
            m_temperature = 0.0f;
        m_humidity = m_dht.getHumidity();
        if (isnan(m_humidity))
            m_humidity = 0.0f;
    }

    return m_temperature;
}

float Data::get_humidity()
{
    // read dht sensor data
    get_temperature();
    return m_humidity;
}

float Data::get_intVoltage()
{
    // read internal voltage
    const auto currentTime = millis();
    if (currentTime - m_battTimeStamp >= m_battWaitTime)
    {
        m_battTimeStamp = currentTime;
        m_intvoltage = float(analogRead(m_tlmSettings.hall_sensor_pin)) / 4095 * 2 * 3.3 * 1.1;
    }
    return m_intvoltage;
}

int Data::get_battPercent()
{
    get_intVoltage();
    m_battPercent = 100 * (m_intvoltage - 3.3) / (4.2 - 3.3);
    if (m_battPercent > 100)
        m_battPercent = 100;
    if (m_battPercent < 0)
        m_battPercent = 0;

    return m_battPercent;
}

uint8_t Data::get_aprsPacketSeq()
{
    return m_aprsPacketSeq;
}

void Data::set_aprsPacketSeq(uint8_t count)
{
    m_aprsPacketSeq = count;
}

void Data::inc_aprsPacketSeq()
{
    ++m_aprsPacketSeq;
    if (m_aprsPacketSeq > 999)
        m_aprsPacketSeq = 0;
}

bool Data::get_statusPCUSBpower()
{
#if SERIALDEBUG
    // Serial.println("{Data::get_statusPCUSBPower}");
#endif
    m_statusPCUSBpower = digitalRead(m_tlmSettings.usb_power_pin);
    return m_statusPCUSBpower;
}

bool Data::get_statusMainsPower()
{
#if SERIALDEBUG
    // Serial.println("{Data::get_statusMainsPower}");
#endif
    m_statusMainsPower = digitalRead(m_tlmSettings.mains_power_pin);
    return m_statusMainsPower;
}

bool Data::get_statusChanged()
{
    auto statusChanged =
        ((m_previousStatusEchoLink != m_statusEchoLink) || (m_previousStatusMainsPower != m_statusMainsPower) ||
         (m_previousStatusPCconnected != m_statusPCconnected) || (m_previousStatusUpLink != m_statusUpLink) ||
         (m_previousStatusUSBPower != m_statusPCUSBpower));
#if SERIALDEBUG
    if (statusChanged)
        Serial.println("{Data::get_statusChanged} true");
#endif
    reset_statusChanged();
    return statusChanged;
}

void Data::reset_statusChanged()
{
    m_previousStatusEchoLink = m_statusEchoLink;
    m_previousStatusMainsPower = m_statusMainsPower;
    m_previousStatusPCconnected = m_statusPCconnected;
    m_previousStatusUpLink = m_statusUpLink;
    m_previousStatusUSBPower = m_statusPCUSBpower;
}

bool Data::get_statusPCConnected()
{
    return m_statusPCconnected;
}

void Data::set_statusPCConnected(bool pcconnected)
{
    m_statusPCconnected = pcconnected;
}

bool Data::get_statusUpLink()
{
    return m_statusUpLink;
}

void Data::set_statusUpLink(bool uplink_reachable)
{
    m_statusUpLink = uplink_reachable;
}

bool Data::get_statusEchoLink()
{
    return m_statusEchoLink;
}

void Data::set_statusEchoLink(bool echolink_logged_in)
{
    m_statusEchoLink = echolink_logged_in;
}

void Display::init()
{
#if SERIALDEBUG
    Serial.println("Display::init");
#endif

    Data::init();
    m_lcd.init();
    m_lcd.flipScreenVertically();
    m_lcd.setBrightness(67);
    char tmpStr[30]{""};
    m_lcd.clear();
    m_lcd.setTextAlignment(TEXT_ALIGN_LEFT);
    m_lcd.setFont(ArialMT_Plain_10);
    m_lcd.setColor(WHITE);
    strcat(tmpStr, m_txt.app_title.c_str());
    strcat(tmpStr, " ");
    strcat(tmpStr, m_basicSettings.version.c_str());
    m_lcd.drawString(0, 0, tmpStr);
    m_lcd.drawHorizontalLine(0, 11, 128);
    m_lcd.display();
}

/**
 * @brief Draws a string inside a box at the given location
 *
 * @param x x-position
 * @param y y-position
 * @param width width of the box
 * @param height height of the box
 * @param text text
 * @param inverse print inverse
 */
void Display::printBox(int16_t x, int16_t y, int16_t width, int16_t height, const String &text, bool inverse)
{
    m_lcd.setColor(inverse ? WHITE : BLACK);
    m_lcd.fillRect(x, y, width, height);
    m_lcd.setColor(inverse ? BLACK : WHITE);
    m_lcd.drawRect(x, y, width, height);
    m_lcd.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    m_lcd.drawString(x + (width >> 1), y + (height >> 1), text); // >> 1 eq divide by 2
}

void Display::displayData()
{
#if SERIALDEBUG
    // Serial.println("{Display::displayData}");
#endif
    char tmpStr[30]{""};
    m_lcd.clear();
    m_lcd.setTextAlignment(TEXT_ALIGN_LEFT);
    m_lcd.setFont(ArialMT_Plain_10);
    m_lcd.setColor(WHITE);

    strcat(tmpStr, m_txt.app_title.c_str());
    strcat(tmpStr, " ");
    strcat(tmpStr, m_basicSettings.version.c_str());
    m_lcd.drawString(0, 0, tmpStr);

    sprintf(tmpStr, m_txt.battery.c_str(), m_intvoltage, m_battPercent);
    m_lcd.drawString(0, 13, tmpStr);
#if SERIALDEBUG
    // Serial.println(tmpStr);
#endif

    sprintf(tmpStr, m_txt.temp_hum.c_str(), m_temperature, m_humidity);
    m_lcd.drawString(0, 24, tmpStr);
#if SERIALDEBUG
    // Serial.println(tmpStr);
#endif

    sprintf(tmpStr, m_txt.usb_pwr.c_str());
    printBox(0, 37, 62, 13, tmpStr, m_statusPCUSBpower);
#if SERIALDEBUG
    // Serial.println(tmpStr);
#endif

    sprintf(tmpStr, m_txt.ext_pwr.c_str());
    printBox(63, 37, 127 - 63, 13, tmpStr, m_statusMainsPower);
#if SERIALDEBUG
    // Serial.println(tmpStr);
#endif

    sprintf(tmpStr, m_txt.pc_conn.c_str());
    printBox(0, 50, 42, 13, tmpStr, m_statusPCconnected);
#if SERIALDEBUG
    // Serial.println(tmpStr);
#endif

    sprintf(tmpStr, m_txt.net_uplink.c_str());
    printBox(42, 50, 42, 13, tmpStr, m_statusUpLink);

#if SERIALDEBUG
    // Serial.println(tmpStr);
#endif

    sprintf(tmpStr, m_txt.net_echolink.c_str());
    printBox(84, 50, 42, 13, tmpStr, m_statusEchoLink);
#if SERIALDEBUG
    // Serial.println(tmpStr);
#endif
    m_lcd.display();
}

#include <LoRa.h> // LoRa library by Sandeep Mistry
#include <config.h>
#include <hb9gl.h>
#include <string>

class MyLora : public LoRaClass
{
public:
    void init();
    void tx(String tx_data);
    template <typename T>
    void tx(T tx_data);
    void tx_telemetry_beacon(Display &display);
    void tx_telemetry_data(Display &display);

private:
    Settings m_settings;
    std::string lpad(std::string const &str, size_t length, char paddedChar = ' ');
    std::string rpad(std::string const &str, size_t s, char paddedChar = ' ');
};

# HB9GL-R Monitoring LoRa Module

Goal of this project was to get some system status of the HB9GL-R VHF relay when the internet link is off or the PC is unreachable.

The Module sends status data to the APRS network every 15 minutes or when critical signals/voltages changes.

I just made this repository visible to share my work. It's not code optimized in any kind, nor is it "clean" code.
As soon as I find time, I will continue working on it.

The actual module is installed since October 2024 and is sending data to the APRS network. You can watch it here: https://aprs.fi/info/a/HB9GL-15

## Flashing via ESP Flash Download Tool from Espressif

This is the upload list for ESP Flash Download Tool

| Adress  | File                                                                                                                                   |
| :------ | :------------------------------------------------------------------------------------------------------------------------------------- |
| 0x1000  | "%%USER%%\OneDrive\Documents\SoftDev\HB9GL-R Monitoring\HB9GL-R Monitoring LoRa Module\.pio\build\ttgo-lora32-v1\bootloader.bin" |
| 0x8000  | "%%USER%%\OneDrive\Documents\SoftDev\HB9GL-R Monitoring\HB9GL-R Monitoring LoRa Module\.pio\build\ttgo-lora32-v1\partitions.bin" |
| 0xe000  | "%%USER%%\.platformio\packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin"                                      |
| 0x10000 | "%%USER%%\OneDrive\Documents\SoftDev\HB9GL-R Monitoring\HB9GL-R Monitoring LoRa Module\.pio\build\ttgo-lora32-v1\firmware.bin"   |

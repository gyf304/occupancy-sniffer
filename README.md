# Occupancy Sniffer

A part of CMU Library Occupancy project. For ESP-8266 Series SoCs. 

## Configuration

Configuration file should be stored as `user/config.h`. An example is provided at `user/config.example.h`.

### Wi-Fi Authentication

Currently WPA/WPA2 Enterprise is not supported. Fill in SSID at `WIFI_SSID` and password at `WIFI_PASSWORD`. Leave `WIFI_PASSWORD` as an empty string if encryption is not enabled.

### XTEA-CCM Encryption

Please change both `ENCRYPT_KEY` and `AUTH_KEY` to randomly generated octets. Note that for proper encryption and authentication, two should never be the same. Same values should be used on the server.

### API Settings

Change `API_HOSTNAME` and `API_PORT` to match server. If using the supplied sever, the default `API_PATH` value should be correct.

### Sniff Settings

Change `SNIFF_CHANNEL` to desired sniffing channel. Suggested values are 1, 6, 11.
Change `SNIFF_TIME` to desired value. Shorter `SNIFF_TIME` increases frequency of reports to server but also increases dead time. (Sniffing is disabled while reporting.)

### Misc Settings

It is not recommended to change other settings.

## Build Instructions

### Prerequisites

  - ESP8266 Open-SDK
  - ESPTOOL from Espressif
  - Serial Bridge Driver

### Build Configuration

Open `Makefile` and

  - Point `XTENSA_TOOLS_ROOT` to the root of ESP8266 tool-chain.
  - Point `SDK_BASE` to the root of ESP8266 SDK.
  - Point `ESPPORT` to the port of the serial bridge.
  - Point `ESPTOOL` to ESPTOOL.
  - Set `FW_FILE_1_ADDR`, `FW_FILE_1_ADDR`, `FW_INIT_DATA_DEFAULT_ADDR` and `FW_BLANK_ADDR` to ones matching flash configuration.


### Building and Flashing

  - Issue `make clean; make` for a clean build
  - Issue `make get-mac` to get mac addr of ESP8266
  - Issue `make erase` to erase ESP8266
  - Issue `make flash-init` to initialize ESP8266
  - Issue `make flash` to flash ESP8266
  - Issue `make tty` to monitor ESP8266 serial. Use `Ctrl-]` to quit.

FROM ubuntu:xenial

ARG ESP8266_RTOS_SDK=8a05097c254a6762947ba0cbd3095e8d901965b7

RUN apt-get update && apt-get install -y \
  bash curl git wget unzip bzip2 xz-utils \
  make srecord bc gcc \
  python python-serial

RUN adduser --system --group --home /build build
RUN chown build /opt && install -o build -d /opt/bin
USER build

RUN curl -L https://github.com/nodemcu/nodemcu-firmware/raw/2.1.0-master_20170811/tools/esp-open-sdk.tar.xz | tar -C /opt -xJv
RUN curl -L https://github.com/espressif/ESP8266_RTOS_SDK/archive/${ESP8266_RTOS_SDK}.tar.gz | tar -C /opt -xzv
RUN curl -L https://raw.githubusercontent.com/espressif/esptool/v2.0.1/esptool.py -o /opt/bin/esptool.py && chmod +x /opt/bin/esptool.py

ENV \
  PATH=/opt/bin:/opt/esp-open-sdk/xtensa-lx106-elf/bin:$PATH \
  SDK_PATH=/opt/ESP8266_RTOS_SDK-${ESP8266_RTOS_SDK} \
  BIN_PATH=/esp/bin

RUN ln -s $SDK_PATH/bin /opt/bin/sdk

ENV \
  BOOT=none APP=0 \
  SPI_SPEED=40 SPI_MODE=DIO SPI_SIZE_MAP=6

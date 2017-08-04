FROM ubuntu:xenial

RUN apt-get update && apt-get install -y \
  bash curl git wget unzip bzip2 \
  make unrar-free autoconf automake libtool libtool-bin gcc g++ gperf \
  gawk flex bison sed \
  help2man texinfo \
  ncurses-dev libexpat-dev \
  python-dev python python-serial

RUN adduser --system --group --home /build build
RUN chown build /opt
USER build

RUN git clone --recursive https://github.com/pfalcon/esp-open-sdk.git /opt/esp-open-sdk

WORKDIR /opt/esp-open-sdk
RUN make STANDALONE=n

RUN curl -L https://github.com/espressif/ESP8266_RTOS_SDK/archive/v1.4.0.tar.gz | tar -C /opt -xzv

ENV \
  PATH=/opt/esp-open-sdk/xtensa-lx106-elf/bin:$PATH \
  SDK_PATH=/opt/ESP8266_RTOS_SDK-1.4.0 \
  BIN_PATH=/esp/bin

ENV \
  BOOT=none APP=0 \
  SPI_SPEED=40 SPI_MODE=DIO SPI_SIZE_MAP=7

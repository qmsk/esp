FROM debian:buster

RUN apt-get update && apt-get install -y \
  bash curl git wget unzip bzip2 xz-utils \
  make srecord bc gcc \
  python python-serial \
  python3 python3-serial

RUN adduser --system --group --home /build build
RUN chown build /opt && install -o build -d /opt/bin
USER build

ARG TOOLCHAIN_VERSION=linux64-1.22.0-88-gde0bdc1-4.8.5
ARG TOOLCHAIN_SHA256SUM=08486ae82e6349b61b83bb2c66f706718638523bd71810f2f2d7b3d17b829a7a

RUN \
	curl -fsSL https://dl.espressif.com/dl/xtensa-lx106-elf-${TOOLCHAIN_VERSION}.tar.gz > /tmp/xtensa-lx106-elf-${TOOLCHAIN_VERSION}.tar.gz && \
	echo "${TOOLCHAIN_SHA256SUM} /tmp/xtensa-lx106-elf-${TOOLCHAIN_VERSION}.tar.gz" | sha256sum -c && \
	tar -C /opt -xzf /tmp/xtensa-lx106-elf-${TOOLCHAIN_VERSION}.tar.gz

ARG ESP8266_RTOS_SDK_VERSION=2.1.1
ARG ESP8266_RTOS_SDK_SHA256SUM=f161eb892329fc76e90574929eb39c135828e7b55b4ea490e74a64ccbde9de3a

RUN \
	curl -fsSL https://github.com/espressif/ESP8266_RTOS_SDK/archive/v${ESP8266_RTOS_SDK_VERSION}.tar.gz > /tmp/ESP8266_RTOS_SDK-${ESP8266_RTOS_SDK_VERSION}.tar.gz && \
	echo "${ESP8266_RTOS_SDK_SHA256SUM} /tmp/ESP8266_RTOS_SDK-${ESP8266_RTOS_SDK_VERSION}.tar.gz" | sha256sum -c && \
	tar -C /opt -xzf /tmp/ESP8266_RTOS_SDK-${ESP8266_RTOS_SDK_VERSION}.tar.gz

ARG ESPTOOL_VERSION=2.8

RUN curl -fsSL https://raw.githubusercontent.com/espressif/esptool/v${ESPTOOL_VERSION}/esptool.py -o /opt/bin/esptool.py && chmod +x /opt/bin/esptool.py

ENV \
  PATH=/opt/bin:/opt/xtensa-lx106-elf/bin:$PATH \
  SDK_PATH=/opt/ESP8266_RTOS_SDK-${ESP8266_RTOS_SDK_VERSION} \
  BIN_PATH=/opt/bin

RUN ln -s $SDK_PATH/bin /opt/bin/sdk

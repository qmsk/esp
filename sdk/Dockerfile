FROM debian:buster

# dependencies
RUN apt-get update && apt-get install -y \
  bash curl git wget unzip bzip2 xz-utils \
  libncurses-dev flex bison \
  make srecord bc gcc gperf \
  python python-pip python-serial

# toolchain
# https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/index.html#get-esp8266-rtos-sdk
ARG TOOLCHAIN_VERSION=gcc8_4_0-esp-2020r3
ARG TOOLCHAIN_SHA256SUM=0a1804b5e2231c6db8b72af6bc2a0f9a5b6994cfba29956d412651109f13fe7e

RUN \
	curl -fsSL https://dl.espressif.com/dl/xtensa-lx106-elf-${TOOLCHAIN_VERSION}-linux-amd64.tar.gz > /tmp/xtensa-lx106-elf-${TOOLCHAIN_VERSION}-linux-amd64.tar.gz && \
	echo "${TOOLCHAIN_SHA256SUM} /tmp/xtensa-lx106-elf-${TOOLCHAIN_VERSION}-linux-amd64.tar.gz" | sha256sum -c && \
	tar -C /opt -xzf /tmp/xtensa-lx106-elf-${TOOLCHAIN_VERSION}-linux-amd64.tar.gz

ENV PATH=/opt/bin:/opt/xtensa-lx106-elf/bin:$PATH

# SDK
# https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/index.html#get-esp8266-rtos-sdk
ARG ESP8266_RTOS_SDK_VERSION

RUN \
  git clone --branch $ESP8266_RTOS_SDK_VERSION --depth 1 https://github.com/espressif/ESP8266_RTOS_SDK.git /opt/ESP8266_RTOS_SDK && \
  git -C /opt/ESP8266_RTOS_SDK submodule update --init --recursive

ENV IDF_PATH=/opt/ESP8266_RTOS_SDK

# custom patches
COPY patches/ /tmp/patches
RUN for patch in /tmp/patches/*.patch; do patch -d $IDF_PATH -p1 < $patch || exit 1; done

RUN \
  python -m pip install -r $IDF_PATH/requirements.txt

RUN \
  make -C $IDF_PATH/tools/kconfig

# build
RUN adduser --system --group --home /build build
RUN adduser build dialout # flashing
USER build
WORKDIR /build

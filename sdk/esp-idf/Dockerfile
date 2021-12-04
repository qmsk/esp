FROM debian:bullseye

# dependencies
#   https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/linux-setup.html
RUN apt-get update && apt-get install -y \
  git wget flex bison gperf \
  python3 python3-pip python3-setuptools \
  cmake ninja-build ccache dfu-util \
  libffi-dev libssl-dev libusb-1.0-0

# SDK
#   https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#linux-and-macos
ARG ESP_IDF_VERSION
ENV IDF_PATH=/opt/esp-idf

RUN \
  git clone --branch $ESP_IDF_VERSION --depth 1 https://github.com/espressif/esp-idf.git /opt/esp-idf && \
  git -C /opt/esp-idf submodule update --init --recursive

# SDK tools
ARG ESP_IDF_TARGETS=esp32
ENV \
  IDF_TOOLS_PATH=/opt/esp-idf-tools \
  ESP_PYTHON=python3

# install.sh
RUN ${ESP_PYTHON} /opt/esp-idf/tools/idf_tools.py install
RUN ${ESP_PYTHON} /opt/esp-idf/tools/idf_tools.py install-python-env

# export.sh
RUN ${ESP_PYTHON} /opt/esp-idf/tools/idf_tools.py export > /opt/esp-idf-envs.sh
RUN . /opt/esp-idf-envs.sh && ${ESP_PYTHON} /opt/esp-idf/tools/check_python_dependencies.py

ADD entrypoint.sh /opt/esp-idf-entrypoint.sh

ENTRYPOINT ["/opt/esp-idf-entrypoint.sh"]

# build
RUN adduser --system --group --home /build build
RUN adduser build dialout # flashing

USER build
WORKDIR /build

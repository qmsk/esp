# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html

# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# use common esp8266/esp32 main + components
set(EXTRA_COMPONENT_DIRS "../../main")
set(COMPONENT_DIRS $ENV{IDF_PATH}/components "../../components" ${EXTRA_COMPONENT_DIRS})

# exclude broken components:
# * wifi_provisioning: conflicts with custom json module
set(EXCLUDE_COMPONENTS "wifi_provisioning")

# use custom sdkconfig defaults from BOARDCONFIG env
set(BOARDCONFIG "$ENV{BOARDCONFIG}")
if(NOT BOARDCONFIG)
  set(BOARDCONFIG "qmsk-esp32")
endif()

set(SDKCONFIG "sdkconfig@${BOARDCONFIG}")
set(SDKCONFIG_DEFAULTS "sdkconfig.defaults;boardconfig/${BOARDCONFIG}")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(qmsk-esp32)

# web-dist
spiffs_create_partition_image(web-dist ../../web/dist FLASH_IN_PROJECT)

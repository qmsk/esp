# @see https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/api-guides/build-system.html

PROJECT_NAME := qmsk-esp
EXTRA_COMPONENT_DIRS :=
COMPONENT_DIRS := $(abspath ../../components) ${EXTRA_COMPONENT_DIRS} ${IDF_PATH}/components $(abspath ../../main)

include $(IDF_PATH)/make/project.mk

SPIFFS_IMAGE_FLASH_IN_PROJECT := 1
SPIFFS_IMAGE_DEPENDS := ../../web/dist/*
$(eval $(call spiffs_create_partition_image,web-dist,../../web/dist))

write_flash_sh: partition_table_get_info blank_ota_data
	echo esptool.py write_flash $(ESPTOOL_WRITE_FLASH_OPTIONS) $(ESPTOOL_ALL_FLASH_ARGS) | sed -e 's:/'$(ESPTOOLPY_SRC)':esptool.py:g' -e 's:'$(BUILD_DIR_BASE)/'::g' > $(BUILD_DIR_BASE)/esptool-write-flash.sh

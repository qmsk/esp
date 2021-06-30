# @see https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/api-guides/build-system.html

PROJECT_NAME := qmsk-esp

include $(IDF_PATH)/make/project.mk

SPIFFS_IMAGE_FLASH_IN_PROJECT := 1
SPIFFS_IMAGE_DEPENDS := web/dist/*
$(eval $(call spiffs_create_partition_image,web-dist,web/dist))

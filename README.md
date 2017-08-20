## Building

  $ docker build -f sdk/Dockerfile -t esp8266/sdk sdk/
  $ docker run -it --rm --name esp8266-build -v $PWD:/build -e BIN_PATH=/build/bin -w /build -u $UID esp8266/sdk make
  $ docker run -it --rm --name esp8266-flash -v $PWD:/build -e BIN_PATH=/build/bin -w /build -u root --device /dev/ttyUSB5 esp8266/sdk esptool.py --port /dev/ttyUSB5 write_flash -ff 40m -fm dio 0x00000 bin/eagle.flash.bin 0x20000 bin/eagle.irom0text.bin 0x3FC000 bin/sdk-1.5.0/esp_init_data_default.bin 0x3FE000 bin/sdk-1.5.0/blank.bin
  $ docker run -it --rm --name esp8266-flash -v $PWD:/build -e BIN_PATH=/build/bin -w /build -u root --device /dev/ttyUSB5 esp8266/sdk esptool.py --port /dev/ttyUSB5 write_flash -ff 40m -fm dio 0x00000 bin/eagle.flash.bin 0x20000 bin/eagle.irom0text.bin

  $ docker run -it --rm --name esp8266-console -v $PWD:/build -e BIN_PATH=/build/bin -w /build -u root --device /dev/ttyUSB5 -e LC_ALL=C.UTF-8 esp8266/sdk miniterm.py /dev/ttyUSB5 74880

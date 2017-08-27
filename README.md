Using the [Espressif ESP8266 RTOS SDK](https://github.com/espressif/ESP8266_RTOS_SDK) (v1.5.0)

## Usage

```
> help
 help: Show this listing
 test [...]
 config version: Version
 config wifi.ssid [SSID]: WiFi SSID
 config wifi.password [PASSWORD]: WiFi Password
 wifi status
 wifi scan
 dmx zero COUNT: Send COUNT channels at zero
 dmx all COUNT VALUE: Send COUNT channel at VALUE
 dmx values [VALUE [...]]: Send channels from VALUE...
```

## Features

### `user/uart`

Synchronous `read`/`write` using interrupts and FreeRTOS queues.

UART RX interrupt handler sends to queue, and TX interrupt handler receives from queue.

Writes bypass the TX queue/interrupt if there is room in the hardware FIFO.

### `user/logging`

Redirect `OS` logging, and provide `DEBUG` / `INFO` / `WARN` / `ERROR` logging with function prefix.

### `user/cli`

Basic line buffering from UART, evaluating input lines via `lib/cmd`.

Support for the following ASCII control codes:

* `\r` (ignored)
* `\n` (end of line)
* `\b` (wipeout)

### `lib/cmd`

CLI commands with arguments, subcommands.

### `user/config`

Basic SPIFFS-based persistent configuration.

TODO: save/reload configuration

### `user/wifi`

Configure WIFI STA from loaded configuration (SSID, PSK).

#### `wifi status`
```
> wifi status
wifi mode=1 phymode=3
wifi sta mac=18:fe:34:d6:0a:4b
wifi sta ssid=qmsk-iot24 password=O...e
wifi sta rssi=-64
wifi sta status=5
wifi sta ip=192.168.2.108
wifi dhcp status=1 hostname=ESP_D60A4B
```

#### `wifi scan`
```
wifi scan:                             SSID             BSSID CHAN RSSI
wifi scan:                 AAAAAAAAAAAAAAAA xx:xx:xx:xx:xx:xx    1  -91
wifi scan:                     BBBBBBBBBBBB xx:xx:xx:xx:xx:xx   11  -60
wifi scan:                       CCCCCCCCCC xx:xx:xx:xx:xx:xx    6  -69
wifi scan:                        DDDDDDDDD xx:xx:xx:xx:xx:xx    6  -79
wifi scan:          EEEEEEEEEEEEEEEEEEEEEEE xx:xx:xx:xx:xx:xx    6  -86
wifi scan:                       qmsk-iot24 xx:xx:xx:xx:xx:xx   11  -54
wifi scan:                       FFFFFFFFFF xx:xx:xx:xx:xx:xx   11  -54
wifi scan: total of 7 APs
```

### `user/dmx`

WIP UART1 DMX output.

## Building

    $ docker build -f sdk/Dockerfile -t esp8266/sdk sdk/
    $ docker run -it --rm --name esp8266-build -v $PWD:/build -e BIN_PATH=/build/bin -w /build -u $UID esp8266/sdk make

    $ docker run -it --rm --name esp8266-flash -v $PWD:/build -e BIN_PATH=/build/bin -w /build -u root --device /dev/ttyUSB5 esp8266/sdk esptool.py --port /dev/ttyUSB5 write_flash -ff 40m -fm dio 0x00000 bin/eagle.flash.bin 0x20000 bin/eagle.irom0text.bin 0x3FC000 bin/sdk-1.5.0/esp_init_data_default.bin 0x3FE000 bin/sdk-1.5.0/blank.bin
    $ docker run -it --rm --name esp8266-flash -v $PWD:/build -e BIN_PATH=/build/bin -w /build -u root --device /dev/ttyUSB5 esp8266/sdk esptool.py --port /dev/ttyUSB5 write_flash -ff 40m -fm dio 0x00000 bin/eagle.flash.bin 0x20000 bin/eagle.irom0text.bin

    $ docker run -it --rm --name esp8266-console -v $PWD:/build -e BIN_PATH=/build/bin -w /build -u root --device /dev/ttyUSB5 -e LC_ALL=C.UTF-8 esp8266/sdk miniterm.py /dev/ttyUSB5 74880

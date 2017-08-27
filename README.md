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

```
!       SDK version 1.5.0-dev(1474356)
|       ESP8266 chip ID=0xd60a4b
|       Reset reason=6 (exc cause=0)
|       Reset exc: cause=0 epc1=00000000 epc2=00000000 epc3=00000000 excv=00000000 depc=00000000 rtn=00000000
|       Boot version=31 mode=1 addr=0
|       Flash id=1458400 size_map=4
|       CPU freq=80
|       Heap free=51256
|       Memory info:
data  : 0x3ffe8000 ~ 0x3ffe893a, len: 2362
rodata: 0x3ffe8940 ~ 0x3ffe8c20, len: 736
bss   : 0x3ffe8c20 ~ 0x3ffefc50, len: 28720
heap  : 0x3ffefc50 ~ 0x40000000, len: 66480
*---------------------------------------
INFO  init_uart: setup baud=74880
INFO  init_logging: initialized
INFO  init_spiffs: size=100000 flash=2fb000:3fafff
INFO  read_config: read
INFO  load_config: version=2
INFO  init_cli:
INFO  init_wifi: config station mode with ssid=qmsk-iot24
OS: mode : sta(18:fe:34:d6:0a:4b)
OS: add if0
INFO  cli_task: init cli=0x3ffef234
> OS: scandone
OS: state: 0 -> 2 (b0)
OS: state: 2 -> 3 (0)
OS: state: 3 -> 5 (10)
OS: add 0
OS: aid 1
OS: pm open phy_2,type:2 0 0
OS: cnt
OS:
OS: connected with qmsk-iot24, channel 11
OS: dhcp client start...
INFO  on_wifi_event: connected: ssid=qmsk-iot24 bssid=xx:xx:xx:xx:xx:xx channel=11
OS: ip:192.168.2.108,mask:255.255.255.0,gw:192.168.2.1
INFO  on_wifi_event: got ip: ip=192.168.2.108 mask=255.255.255.0 gw=192.168.2.1
```

### `user/cli`

Basic line buffering from UART, evaluating input lines via `lib/cmd`.

Support for the following ASCII control codes:

* `\r` (ignored)
* `\n` (end of line)
* `\b` (wipeout)

### `lib/cmd`

CLI commands with arguments, subcommands, usage help.

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

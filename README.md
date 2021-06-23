Using the [Espressif ESP8266 RTOS SDK](https://github.com/espressif/ESP8266_RTOS_SDK) (v3.4)

# Build

Prepare the SDK/toolchain:

    $ docker-compose build sdk

Build firmware image:

    $ USER_ID=$UID docker-compose run --rm build

# Install

Flash to correct nodeMCU device:

    TTY_DEV=/dev/ttyUSB? docker-compose run --rm flash

# Usage

Connect to the serial console:

    TTY_DEV=/dev/ttyUSB? docker-compose run --rm monitor

```
> help
 help: Show this listing
 echo [...]
 led off: Turn off LED
 led slow: Blink LED slowly
 led fast: Blink LED fast
 led blink: Blink LED once
 led on: Turn on LED
 config show [SECTION]: Show config settings
 config get SECTION NAME: Get config setting
 config set SECTION NAME VALUE: Set and write config
 config reset: Remove stored config and reset to defaults
 wifi status
 wifi scan
 dmx zero COUNT: Send COUNT channels at zero
 dmx all COUNT VALUE: Send COUNT channel at VALUE
 dmx values [VALUE [...]]: Send channels from VALUE...
 spi setup MODE CLOCK-DIV: Setup SPI master
 spi send BITS:COMMAND BITS:HEX-ADDRESS BITS:STRING-DATA: Send op
 spi write [BYTE [...]]: Send bytes
 p9813 set INDEX RGB: Set values
 p9813 off : Power off
> config show
[wifi]
ssid = qmsk-iot24
password = ***

[artnet]
universe = 0

[dmx]
gpio = 4
artnet_universe = 1

[p9813]
count = 0
artnet_universe = 0
gpio = 0
```

## Features

### `lib/uart`

Synchronous `read`/`write` using interrupts and FreeRTOS queues.

UART RX interrupt handler sends to queue, and TX interrupt handler receives from queue.

Writes bypass the TX queue/interrupt if there is room in the hardware FIFO.

### `lib/logging`

Redirect `OS` logging, and provide `DEBUG` / `INFO` / `WARN` / `ERROR` logging with function prefix.

### `lib/cli`

Basic line buffering from UART, evaluating input lines via `lib/cmd`.

Support for the following ASCII control codes:

* `\r` (ignored)
* `\n` (end of line)
* `\b` (wipeout)

### `lib/cmd`

CLI commands with arguments, subcommands, usage help.

### `lib/config`

Basic SPIFFS-based persistent configuration.

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

DMX output via UART1.

### `user/p9813`

SPI control of chained P9813 LED drivers.

### `user/artnet`

ArtNet UDP receiver for dmx/p9813 control.

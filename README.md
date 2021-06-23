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

## CLI

From `help` output:

```
 help: Show commands
 system info: Print system info
 system status: Print system status
 system tasks: Print system tasks
 system restart: Restart system
 user-led off: Turn off LED
 user-led on: Turn on LED
 user-led slow: Blink LED slowly
 user-led fast: Blink LED fast
 user-led flash: Blink LED once
 spiffs info [LABEL]: Show SPIFFS partition
 spiffs format [LABEL]: Format SPIFFS partition
 vfs ls PATH: List files
 config show [SECTION]: Show config settings
 config get SECTION NAME: Get config setting
 config set SECTION NAME VALUE: Set and write config
 config reset: Remove stored config and reset to defaults
 wifi scan [SSID]: Scan available APs
 wifi connect [SSID] [PSK]: Connect AP
 wifi info : Show connected AP
 spi-leds clear : Clear values
 spi-leds all RGB [A]: Set all pixels to values
 spi-leds set INDEX RGB [A]: Set one pixel to value
 dmx zero COUNT: Output COUNT channels at zero
 dmx all COUNT VALUE: Output COUNT channels at VALUE
 dmx count COUNT: Output COUNT channels with 0..COUNT as value
 dmx out VALUE...: Output given VALUEs as channels
```

## Configuration
From `config show`, `GET /config.ini` output:

```
[activity_led]
enabled = false
gpio = 0
inverted = false

[atx_psu]
enabled = false
gpio = 0
timeout = 10

[wifi]
enabled = true
ssid = qmsk-iot
password = ***

[http]
host = 0.0.0.0
port = 80

[artnet]
enabled = false
universe = 0

[spi_leds]
enabled = true
count = 1
protocol = APA102
artnet_enabled = false
artnet_universe = 0
artnet_mode = BGR

[dmx]
enabled = false
output_enable_gpio = 5
artnet_enabled = false
artnet_universe = 0
```

# Components

## `uart1`

Synchronous TX-only UART1 implementation using interrupts and FreeRTOS queues.

Writes bypass the TX queue and interrupt handler if there is room in the hardware FIFO. UART TX interrupt handler empties the queue.

Supports RS232/458 breaks as required for e.g. DMX resets, outputting break/mark for a specified duration.

## `logging`

Provide `DEBUG` / `INFO` / `WARN` / `ERROR` logging with function context.

## `cli`

Basic line buffering from stdin/stdout, evaluating input lines via `cmd`.

Support for the following ASCII control codes:

* `\r` (ignored)
* `\n` (end of line)
* `\b` (wipeout)

## `cmd`

CLI commands with arguments, subcommands, usage help.

## `config`

Support for reading/writing configuration structs via INI files.

Config read/write from stdio FILE can be used with SPIFFS, CLI and HTTP handlers.

## `status_led`

Simple GPIO LED blinking.

## `spi_leds`

Control SPI-compatible RGB LEDs, with protocol support for:

* APA102/SK9822
* P9813

## `artnet`

Art-NET UDP receiver with support for polling/discovery and multiple DMX outputs with sequence numbering support.

## `json`

Stack-based JSON serializer for stdio output.

## `http`

HTTP protocol support.

## `httpserver`

HTTP server support with listeners, routes, requests and responses.

# Features

## `wifi`

Configure WIFI STA from loaded configuration (SSID, PSK).

### Commands

#### `wifi info`
```
Station e8:db:84:94:5a:7e: Connected
	BSSID               : aa:bb:cc:dd:ee:ff
	SSID                : qmsk-iot
	Channel             : 11:0
	RSSI                : -60
	AuthMode            : WPA2-PSK
	Pairwise Cipher     : NONE
	Group Cipher        : NONE
	Flags               : 11b 11g   
TCP/IP:
	Hostname            : espressif
	DHCP Client         : STARTED
	IP                  : 172.29.16.47
	Netmask             : 255.255.0.0
	Gateway             : 172.29.0.1
	DNS (main)          : 172.29.0.1
	DNS (backup)        : 0.0.0.0
	DNS (fallback)      : 0.0.0.0
```

#### `wifi scan`
```
BSSID            	SSID                            	CH:CH	RSSI	  AUTHMODE	     PAIRW/GROUP CIPHFLAGS
xx:xx:xx:xx:xx:xx	                                	11:0 	-58 	WPA2-PSK  	CCMP      /CCMP      bgn  
aa:bb:cc:dd:ee:ff	qmsk                            	11:0 	-60 	WPA/2-PSK 	TKIP-CCMP /TKIP      bgn  
aa:bb:cc:dd:ee:ff	qmsk-guest                      	11:0 	-61 	WPA/2-PSK 	TKIP-CCMP /TKIP      bgn  
aa:bb:cc:dd:ee:ff	qmsk-iot                        	11:0 	-61 	WPA2-PSK  	CCMP      /CCMP      bgn  
```

### `activity_led`

Blink an activity LED on dmx/spi-leds updates.

### `atx_psu`

Control an ATX PSU `#PS_EN` via GPIO. Powers on the PSU whenever SPI-LED output is active, with a configurable shutdown timeout.

### `artnet`

Art-NET UDP receiver.

Supports up to four Art-NET outputs on the [Art-Net Sub-Net](https://art-net.org.uk/how-it-works/universe-addressing/) matching the higher bits of the configured `universe`. With e.g. `universe = 0`, artnet outputs can use universes 0-15. To use an artnet output universe 16, the `[artnet] universe` must be configured to `16`, and then output universes 16-31 can be used.

### `dmx`

Art-NET DMX output via UART1 TX.

Integrates with `activity_led`.

### `spi_leds`

Art-NET DMX controller for SPI-compatible RGB LEDs.

Integrates with `activity_led` and `atx_psu`.

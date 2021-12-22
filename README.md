Using the [Espressif ESP8266 RTOS SDK](https://github.com/espressif/ESP8266_RTOS_SDK) (v3.4)

# Build

Prepare the SDK/toolchain:

    $ docker-compose build sdk

Build webapp:

    $ USER_ID=$UID docker-compose run --rm npm install
    $ USER_ID=$UID docker-compose run --rm npm run build

Build firmware images (bootloader + app + web-dist):

    $ USER_ID=$UID docker-compose run --rm build

# Flash

Flash to correct nodeMCU device:

    ESPPORT=/dev/ttyUSB? docker-compose run --rm flash

# Usage

By default, the ESP8266 will establish an (open) WiFi Access-Point with a `qmsk-esp-******` name.

Once connected to the `qmsk-esp-******` WiFi network, the integrated Web UI will be available at the matching `http://qmsk-esp-******.local` address.

Please configure a WiFi password, and optionally a HTTP username/password.

## USB Console

By default, the ESP8266 will print log messages and open a CLI console on UART0, which can be accessed via the same USB UART used for bootloader flashing:

    ESPPORT=/dev/ttyUSB? docker-compose run --rm monitor

If using the UART0 pins for other IO (e.g. `spi-leds` I2S interface), the console can be closed manually using the `exit` command,
or automatically at boot using the `timeout` config. The console can be re-started using a short press on the FLASH button;
this will re-initiate any configured `timeout`. The console can also be disabled completely using the `enabled` config.

If using the UART0 peripheral for other IO (e.g. `dmx-input`), the SDK `CONFIG_ESP_CONSOLE_UART_CUSTOM` config must be used to redirect SDK logging output. The UART0 console can still be temporarily activated at boot using the `timeout` config to briefly show logging output and offer the option of opening the CLI, or disabled completely using the `enabled` config.

If the `timeout` config is used, the console will use the UART0 to prompt the user, before timing out and releasing the UART0:

    ! Use [ENTER] to open console

## Status LEDs

Designed for NodeMCU ESP8266 boards with the following fixed-function pins:

* D0 (GPIO16): USER_LED (active-low with pull-up)
* D3 (GPIO0): FLASH_LED (active-low with pull-up) + button
* D8 (GPIO15): ALERT_LED (active-high with pull-down)

The active-low LEDs should be connected from +3.3V to the GPIO pin.
The active-high LEDs should be connected from the GPIO pin to GND.

### User LED

* Off: Boot / reset
* Fast blinking: WiFi connecting
* Slow blinking: WiFi disconnected
* On: WiFi connected

### Flash LED

Flashes for ~10ms on each spi-leds/dmx update.

### Alert LED

* On: Boot failed
* Slow: Missing configuration
* Fast: Invalid configuration

## Flash Button

The FLASH button on GPIO0 is used to trigger config mode or a config reset.

Press the FLASH button briefly until the USER LED starts flashing, and release to enter configuration mode. The UART0 console will be activated if stopped, which will block any I2S output.

Press and hold FLASH button for >5s until the USER LED stops flashing, and the system will reset the configuration and restart.

If the FLASH button is held pressed at app boot, the configuration will not be loaded, and the Alert LED will flash slowly to indicate that the default configuration is active. Note that this only applies on a soft reset, if the FLASH button is held at power reset, the bootloader will enter UART flashing mode.

## CLI

From `help` output:

```
help: Show commands
system info: Print system info
system memory: Print system memory
system partitions: Print system partitions
system status: Print system status
system tasks: Print system tasks
system interfaces: Print system network interfaces
system restart: Restart system
status-leds off: Turn off USER LED
status-leds on: Turn on USER LED
status-leds slow: Blink USER LED slowly
status-leds fast: Blink USER LED fast
status-leds flash: Blink FLASH LED once
status-leds read: Read FLASH button
status-leds alert: Turn on ALERT LED
spiffs info [LABEL]: Show SPIFFS partition
spiffs format [LABEL]: Format SPIFFS partition
vfs ls PATH: List files
config show [SECTION]: Show config settings
config get SECTION NAME: Get config setting
config set SECTION NAME VALUE: Set and write config
config clear SECTION NAME: Clear and write config
config reset: Remove stored config and reset to defaults
wifi scan [SSID]: Scan available APs
wifi connect [SSID] [PSK]: Connect AP
wifi info : Show connected AP
spi-leds clear : Clear all output values
spi-leds all RGB [A]: Set all output pixels to value
spi-leds set OUTPUT INDEX RGB [A]: Set one output pixel to value
dmx zero COUNT: Output COUNT channels at zero on all output
dmx all COUNT VALUE: Output COUNT channels at VALUE on all outputs
dmx out OUTPUT VALUE...: Output given VALUEs as channels on output
dmx count OUTPUT COUNT: Output COUNT channels with 0..COUNT as value
```

## Configuration
From `config show`, `GET /config.ini` output:

```
# Control ATX-PSU based on spi-leds output.
# The ATX-PSU will be powered on when spi-led outputs are active, and powered off into standby mode if all spi-led outputs are idle (zero-valued).
[atx_psu]
enabled = false
gpio = 0
# Power off ATX PSU after timeout seconds of idle
timeout = 10

# WiFi station mode, connecting to an SSID with optional PSK.
# Uses DHCP for IPv4 addressing.
[wifi]
mode = AP # OFF STA [AP] APSTA
# For STA mode: minimum threshold for AP provided auth level
# For AP mode: provided auth level
auth_mode = WPA2-PSK # OPEN WEP WPA-PSK [WPA2-PSK] WPA-WPA2-PSK WPA3-PSK WPA2-WPA3-PSK
# For STA mode: connect to AP with given SSID
# For AP mode: start AP with given SSID, or use default
ssid =
password = ***
hostname =

# HTTP API + Web frontend with optional HTTP basic authentication.
[http]
enabled = true
host = 0.0.0.0
port = 80
# Optional HTTP basic authentication username/password
username =
# Optional HTTP basic authentication username/password
password = ***

# Art-Net receiver on UDP port 6454.
# Art-Net addresses consist of the net (0-127) + subnet (0-15) + universe (0-15). All outputs share the same net/subnet, each output uses a different universe. Up to four outputs are supported.
[artnet]
enabled = false
# Set network address, 0-127.
net = 0
# Set sub-net address, 0-16.
subnet = 0

# Control LEDs using synchronous (separate clock/data) serial protocols via Art-Net.
# Multiple serial outputs can be multiplexed from the same SPI driver by using GPIOs to control an external driver chip with active-high/low output-enable GPIO lines.
[spi-leds0]
enabled = false
protocol = APA102 # [APA102] P9813
# Longer cable runs can be noisier, and may need a slower rate to work reliably.
rate = 1M # 20M 10M 5M 2M [1M] 500K 200K 100K 50K 20K 10K 1K
# Delay data signal transitions by system clock cycles to offset clock/data transitions and avoid coupling glitches.
delay = 0
count = 0
# Multiplex between multiple active-high/low GPIO-controlled outputs
gpio_mode = OFF # [OFF] HIGH LOW
# GPIO pin to activate when transmitting on this output
gpio_pin = 0
artnet_enabled = false
# Output from artnet universe (0-15) within [artnet] net/subnet.
artnet_universe = 0
# Art-Net DMX channel mode
artnet_mode = BGR # RGB [BGR] GRB

# Control LEDs using synchronous (separate clock/data) serial protocols via Art-Net.
# Multiple serial outputs can be multiplexed from the same SPI driver by using GPIOs to control an external driver chip with active-high/low output-enable GPIO lines.
[spi-leds1]
enabled = false
protocol = APA102 # [APA102] P9813
# Longer cable runs can be noisier, and may need a slower rate to work reliably.
rate = 1M # 20M 10M 5M 2M [1M] 500K 200K 100K 50K 20K 10K 1K
# Delay data signal transitions by system clock cycles to offset clock/data transitions and avoid coupling glitches.
delay = 0
count = 0
# Multiplex between multiple active-high/low GPIO-controlled outputs
gpio_mode = OFF # [OFF] HIGH LOW
# GPIO pin to activate when transmitting on this output
gpio_pin = 0
artnet_enabled = false
# Output from artnet universe (0-15) within [artnet] net/subnet.
artnet_universe = 0
# Art-Net DMX channel mode
artnet_mode = BGR # RGB [BGR] GRB

# Control LEDs using synchronous (separate clock/data) serial protocols via Art-Net.
# Multiple serial outputs can be multiplexed from the same SPI driver by using GPIOs to control an external driver chip with active-high/low output-enable GPIO lines.
[spi-leds2]
enabled = false
protocol = APA102 # [APA102] P9813
# Longer cable runs can be noisier, and may need a slower rate to work reliably.
rate = 1M # 20M 10M 5M 2M [1M] 500K 200K 100K 50K 20K 10K 1K
# Delay data signal transitions by system clock cycles to offset clock/data transitions and avoid coupling glitches.
delay = 0
count = 0
# Multiplex between multiple active-high/low GPIO-controlled outputs
gpio_mode = OFF # [OFF] HIGH LOW
# GPIO pin to activate when transmitting on this output
gpio_pin = 0
artnet_enabled = false
# Output from artnet universe (0-15) within [artnet] net/subnet.
artnet_universe = 0
# Art-Net DMX channel mode
artnet_mode = BGR # RGB [BGR] GRB

# Control LEDs using synchronous (separate clock/data) serial protocols via Art-Net.
# Multiple serial outputs can be multiplexed from the same SPI driver by using GPIOs to control an external driver chip with active-high/low output-enable GPIO lines.
[spi-leds3]
enabled = false
protocol = APA102 # [APA102] P9813
# Longer cable runs can be noisier, and may need a slower rate to work reliably.
rate = 1M # 20M 10M 5M 2M [1M] 500K 200K 100K 50K 20K 10K 1K
# Delay data signal transitions by system clock cycles to offset clock/data transitions and avoid coupling glitches.
delay = 0
count = 0
# Multiplex between multiple active-high/low GPIO-controlled outputs
gpio_mode = OFF # [OFF] HIGH LOW
# GPIO pin to activate when transmitting on this output
gpio_pin = 0
artnet_enabled = false
# Output from artnet universe (0-15) within [artnet] net/subnet.
artnet_universe = 0
# Art-Net DMX channel mode
artnet_mode = BGR # RGB [BGR] GRB

# DMX output via UART1 -> RS-485 transceiver.
# Because UART1 TX will spew debug messages reset/flash/boot, avoid DMX glitches by using a GPIO pin that is kept low during reset/boot to drive the RS-485 transceiver's active-high transmit/output-enable.
[dmx0]
enabled = false
# GPIO pin will be taken high to enable output once the UART1 TX output is safe.
gpio_pin = 0
# Multiplex between multiple active-high/low GPIO-controlled outputs
gpio_mode = OFF # [OFF] HIGH LOW
artnet_enabled = false
# Output from universe (0-15) within [artnet] net/subnet.
artnet_universe = 0

# DMX output via UART1 -> RS-485 transceiver.
# Because UART1 TX will spew debug messages reset/flash/boot, avoid DMX glitches by using a GPIO pin that is kept low during reset/boot to drive the RS-485 transceiver's active-high transmit/output-enable.
[dmx1]
enabled = false
# GPIO pin will be taken high to enable output once the UART1 TX output is safe.
gpio_pin = 0
# Multiplex between multiple active-high/low GPIO-controlled outputs
gpio_mode = OFF # [OFF] HIGH LOW
artnet_enabled = false
# Output from universe (0-15) within [artnet] net/subnet.
artnet_universe = 0
```

### Configuration Reset

To recover from a broken configuration, either press and hold the FLASH button, or erase the SPIFFS partition using the USB bootloader:

    ESPPORT=/dev/ttyUSB? docker-compose run --rm config-reset

# Components

## `uart1`

Synchronous TX-only UART1 implementation using interrupts and FreeRTOS queues.

Writes bypass the TX queue and interrupt handler if there is room in the hardware FIFO. UART TX interrupt handler empties the queue.

Supports RS232/458 breaks as required for e.g. DMX resets, outputting break/mark for a specified duration.

## `i2s_out`

DMA based I2S output for generating arbitrary (32-bit aligned) bit streams, using interrupts to wait for TX to complete.

Designed for generating WS2812B data signals.

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

## `dmx`

DMX input/output support.

Uses UART0 alternate RTS/CTS output pins for DMX input.

Uses UART1 for DMX output.

## `spi_leds`

Control RGB LEDs using SPI/UART/I2S output interfaces, with protocol support for:

* APA102/SK9822 (SPI)
* P9813 (SPI)
* WS2812B / WS2811 (UART, I2S)
* SK6812-GRBW (UART, I2S)

Supports optional GPIO output for multiplexing a single SPI/UART/I2S output interface between multiple `spi_leds` outputs.

## `artnet`

Art-NET UDP receiver with support for polling/discovery and multiple DMX outputs with sequence numbering support.

Supports local DMX input.

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

### `atx_psu`

Control an ATX PSU `#PS_EN` via GPIO. Powers on the PSU whenever SPI-LED output is active, with a configurable shutdown timeout.

### `artnet`

Art-NET UDP receiver.

Supports up to four Art-NET outputs on the [Art-Net Sub-Net](https://art-net.org.uk/how-it-works/universe-addressing/) matching the higher bits of the configured `universe`. With e.g. `universe = 0`, artnet outputs can use universes 0-15. To use an artnet output universe 16, the `[artnet] universe` must be configured to `16`, and then output universes 16-31 can be used.

### `dmx-input`

Art-NET DMX input via UART2 RX (using UART0 alternate RTS/CTS pins).

The FLASH LED will blink on DMX updates.

Can be used to control local Art-NET outputs using a configured Art-NET universe.

### `dmx-output`

Art-NET DMX output via UART1 TX.

Supports up to two multiplexed outputs using active-high/low GPIOs.

The FLASH LED will blink on DMX updates.

### `spi-leds`

Art-NET DMX controller for RGB LEDs.

Supports up to four multiplexed outouts using active-high/low GPIOs.

The FLASH LED will flash on SPI-LEDs updates.

The ATX PSU output will enable when any SPI-LEDs are active.

The I2S output uses the same IO pins as the UART0 console, and cannot be used while the console is active. Disable the console or set a console timeout to use the I2S output interface.

# HTTP

### `GET /`

Load the integrated Web UI.

### `GET /config.ini`

Returns the boot config in INI form.

### `POST /config.ini`

Load the running config from INI form.

### `GET /api/config`

Returns a JSON structure describing the config schema and current values.

### `POST /api/config`

Accepts `application/x-www-form-urlencoded` form parameters in a `[module]name=value` format, as per `config set ...`.

### `GET /api/system`

Returns a JSON object describing the system info, state and status.

* `info`
* `status`
* `partitions`
* `tasks`
* `interfaces`

### `GET /api/system/tasks`

Refresh the running tasks info.

### `POST /api/system/restart`

Restart the system.

### `GET /api/leds`

Return LED config/state.

### `POST /api/leds`

Accepts `application/x-www-form-urlencoded` form parameters using the following syntax:
* `id=%d` (0-3)
* `all=%x[.%x]` (`RRGGBB` or `RRGGBB.XX`)
* `%u=%x[.%x]` (`RRGGBB` or `RRGGBB.XX`)

The `.XX` suffix is optional, and the interpretation depends on the protocol, per the `GET /api/leds` -> `color_parameter` field:
* `NONE`: not used
* `DIMMER`: 0-255 controls the LED brightness
* `WHITE`: 0-255 controls the white LED

### `GET /api/leds/test`

Returns `[{"mode": ...}]` parameters usable for `POST`.

### `POST /api/leds/test`

Accepts `application/x-www-form-urlencoded` form parameters using the following syntax:

* `id=%d` (0-3)
* `mode=%s` (see `GET /api/leds/test`)

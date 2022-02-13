COMPONENT_SRCDIRS := . ./protocols

ifdef CONFIG_LEDS_I2S_ENABLED
	COMPONENT_SRCDIRS += ./interfaces/i2s
endif

ifdef CONFIG_LEDS_SPI_ENABLED
	COMPONENT_SRCDIRS += ./interfaces/spi
endif

ifdef CONFIG_LEDS_UART_ENABLED
	COMPONENT_SRCDIRS += ./interfaces/uart
endif

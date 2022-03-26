set(src_dirs . ./protocols)

if(CONFIG_LEDS_I2S_ENABLED)
  list(APPEND src_dirs "interfaces/i2s")
endif()

if(CONFIG_LEDS_SPI_ENABLED)
  list(APPEND src_dirs "interfaces/spi")
endif()

if(CONFIG_LEDS_UART_ENABLED)
  list(APPEND src_dirs "interfaces/uart")
endif()

idf_component_register(
  SRC_DIRS "${src_dirs}"
  INCLUDE_DIRS "include"
  REQUIRES driver gpio_out i2s_out stats uart
  PRIV_REQUIRES logging
)

idf_component_register(
  SRC_DIRS . ${IDF_TARGET}
  INCLUDE_DIRS "include"
  PRIV_REQUIRES logging
)

target_compile_options(${COMPONENT_LIB} PRIVATE -O2)

idf_component_register(
  SRC_DIRS . ${IDF_TARGET}
  INCLUDE_DIRS "include"
  PRIV_REQUIRES logging
  LDFRAGMENTS ${IDF_TARGET}.lf
)

target_compile_options(${COMPONENT_LIB} PRIVATE -O2)

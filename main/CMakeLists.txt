idf_component_register(
  SRC_DIRS . ${IDF_TARGET}
)

target_compile_options(${COMPONENT_LIB} PRIVATE -DBOARDCONFIG="${BOARDCONFIG}")

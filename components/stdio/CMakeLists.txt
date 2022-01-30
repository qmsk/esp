idf_component_register(
  SRC_DIRS . ${IDF_TARGET}
  INCLUDE_DIRS "include"
  REQUIRES uart vfs
  PRIV_REQUIRES logging
)

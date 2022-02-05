idf_component_register(
  SRC_DIRS . ${IDF_TARGET}
  INCLUDE_DIRS "include"
  REQUIRES app_update
  PRIV_REQUIRES logging
)

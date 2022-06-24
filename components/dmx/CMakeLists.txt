idf_component_register(
  SRC_DIRS .
  INCLUDE_DIRS "include"
  REQUIRES gpio stats uart
  PRIV_REQUIRES logging
)

idf_component_register(
  SRC_DIRS .
  INCLUDE_DIRS "include"
  PRIV_REQUIRES logging
)

set_source_files_properties(url.c PROPERTIES
  COMPILE_FLAGS -Wno-missing-field-initializers
)

set_source_files_properties(http.c PROPERTIES
  COMPILE_FLAGS -Wno-missing-field-initializers
)

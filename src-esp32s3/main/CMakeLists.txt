idf_component_register(
    SRC_DIRS "."
    SRC_DIRS "./thread"
    SRC_DIRS "./device"
    SRC_DIRS "./model"
    PRIV_INCLUDE_DIRS "."
)

set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")

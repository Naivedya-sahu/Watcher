idf_component_register(
    SRCS "buzzer.cpp"
    INCLUDE_DIRS "."
    REQUIRES driver log
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)

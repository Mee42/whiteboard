idf_component_register(SRCS "hello_world_main.c"
                    EMBED_TXTFILES client.crt client.key
                    INCLUDE_DIRS "")

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")

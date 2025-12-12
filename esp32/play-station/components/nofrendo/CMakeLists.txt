
file(GLOB_RECURSE NOFRENDO_SRCS
        "${CMAKE_CURRENT_LIST_DIR}/*.c"
        "${CMAKE_CURRENT_LIST_DIR}/cpu/*.c"
        "${CMAKE_CURRENT_LIST_DIR}/libsnss/*.c"
        "${CMAKE_CURRENT_LIST_DIR}/mappers/*.c"
        "${CMAKE_CURRENT_LIST_DIR}/nes/*.c"
        "${CMAKE_CURRENT_LIST_DIR}/shdhrdw/*.c"
)

idf_component_register(
        SRCS
        "${NOFRENDO_SRCS}"

        INCLUDE_DIRS
        "./"
        "./cpu/"
        "./libsnss/"
        "./mappers/"
        "./nes/"
        "./sndhrdw"
)
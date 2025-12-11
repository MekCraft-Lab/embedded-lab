
file(GLOB_RECURSE APPLICATIONS_SRCS
        "${CMAKE_CURRENT_LIST_DIR}/applications/*.cpp"
)


idf_component_register(SRCS "main.cpp"
        ${APPLICATIONS_SRCS}

        PRIV_REQUIRES
        lcd-i80
        gamepad
        INCLUDE_DIRS ".")

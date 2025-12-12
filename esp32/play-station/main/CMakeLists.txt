
file(GLOB_RECURSE APPLICATIONS_SRCS
        "${CMAKE_CURRENT_LIST_DIR}/applications/*.cpp"
)


idf_component_register(SRCS
        "main.cpp"
        "sys_call.cpp"
        ${APPLICATIONS_SRCS}

        PRIV_REQUIRES
        lcd-i80
        gamepad
        nofrendo
        INCLUDE_DIRS ".")

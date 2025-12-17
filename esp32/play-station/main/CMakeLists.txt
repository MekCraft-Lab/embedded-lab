
file(GLOB_RECURSE APPLICATIONS_SRCS
        "${CMAKE_CURRENT_LIST_DIR}/applications/*.cpp"
)


idf_component_register(SRCS
        "main.cpp"
        "sys_call.cpp"
        ${APPLICATIONS_SRCS}

        PRIV_REQUIRES
        esp_wifi_remote
        lcd-i80
        nvs_flash
        gamepad
        nofrendo
        INCLUDE_DIRS ".")


file(GLOB_RECURSE APPS_SRCS
        "${CMAKE_CURRENT_LIST_DIR}/Applications/*.cpp"
)

idf_component_register(SRCS "main.cpp"

                    INCLUDE_DIRS "."

        REQUIRES esp_lvgl_port esp_lcd driver lcd-i80
)

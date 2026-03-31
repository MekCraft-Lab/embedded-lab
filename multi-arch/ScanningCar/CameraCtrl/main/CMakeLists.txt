idf_component_register(SRCS "CameraCtrl.c"
                    INCLUDE_DIRS "."
                    REQUIRES freertos driver esp_timer esp_cam_sensor imx219
)

idf_component_register(SRCS "CameraCtrl.c"
        INCLUDE_DIRS "."
        REQUIRES
        freertos
        driver
        esp_timer
        esp_video
        esp_cam_sensor
        nvs_flash    # <--- 必须添加这一行
        esp_event    # 如果你用到了事件循环也建议加上
        esp_netif    # 如果用到了网络接口也建议加上
        esp_video
        esp_driver_jpeg

)
idf_component_register(
  SRCS
    "aegis-swarm.cpp"
    "servo.cpp"
    "ultrasonic.cpp"
    "pir.cpp"
    "swarm.cpp"
  INCLUDE_DIRS "."
  PRIV_REQUIRES
    esp_wifi
    esp_event
    esp_netif
    freertos
    driver
    esp_timer
    nvs_flash 
)
cd .
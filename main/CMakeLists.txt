idf_component_register(
  SRCS
    "aegis-swarm.cpp"
    "servo.cpp"
    "ultrasonic.cpp"
    "pir.cpp"

  INCLUDE_DIRS "."
  REQUIRES driver freertos esp_timer
)

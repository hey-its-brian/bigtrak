set(srcs
        "main.c"
        "main.cpp"
        "battery.cpp"
        "cannon.cpp"
        "cyd_link.cpp"
        "encoders.cpp"
        "gamepad.cpp"
        "motion_test.cpp"
        "motors.cpp"
        "program_runner.cpp"
        "settings.cpp")

set(requires "bluepad32" "bluepad32_arduino" "arduino" "btstack" "esp_driver_pcnt")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "."
                    REQUIRES "${requires}")

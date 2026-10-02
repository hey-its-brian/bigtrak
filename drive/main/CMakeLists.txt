set(srcs
        "main.c"
        "main.cpp"
        "battery.cpp"
        "cannon.cpp"
        "cyd_link.cpp"
        "gamepad.cpp"
        "motion_test.cpp"
        "motors.cpp")

set(requires "bluepad32" "bluepad32_arduino" "arduino" "btstack")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "."
                    REQUIRES "${requires}")

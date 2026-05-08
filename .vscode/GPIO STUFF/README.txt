PICO SDK LIBRARY GPIO FUNCTIONS

*** MUST INCLUDE LIBRARIES ***
- #include "pico/stdlib.h"
- #include "hardware/gpio.h"

gpio_init(pin_number)
    - Takes the gpio pin number
    - Does all of the funky stuff that is awful to code (initialization)

gpio_set_dir(pin_number, GPIO_IN/GPIO_OUT)
    - Takes the gpio pin number and one of two constants, GPIO_IN or GPIO_OUT
    - Sets the resistors, etc. automatically

gpio_pull_up(pin_number)
- Takes the gpio pin number
- Sets the required ports to be pull-up or pull-down resistor
    - Pull Down: 
        - Input = 0 -> Output = 0
    - Pull Up:
        - Input = 0 -> Output = 1

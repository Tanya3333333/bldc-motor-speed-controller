# bldc-motor-speed-controller
Programmed a C-based 8051 MCU to generate PWM for a servo fan, with speed control via an 8-position DIP switch. Measured RPM using encoder edge counting and displayed it on 3 LEDs using lookup tables. Used external interrupts and timer resets; display resets to 000 if RPM exceeds 999.


## What is a firmware library?
* A low level software, that provides low level control access to any device hardware
* Provides a low-level interface for firmware application/software
  
## Why APIs are important in embedded systems?
* To make the system more atomic or modular
* As in embedded systems, it is quite often to communicate with protocol drivers and it is useful if they expose APIs
* Provides an abstraction for hardware/software codesign

## What was understood from the lab code?
The code emulates the general-purpose-input-output (GPIO) behaviour. 
* Initializes the LED pin as output ( can be used for displaying status like blinking or ON/OFF)
* Initializes the Button pin as Input ( to capture the status of use buttons)
* turn-on the LED
* it captures/read the button state
* turn-off the LED
* In-short toggles the LED one time

## Successful compilation
```
wsluser@ISCN5CG4202696:/mnt/c/Users/saleemmehmoo/riscv-internship/vsdsquadron-mini-core/task1$ gcc main.c gpio.c -o task1_demo
wsluser@ISCN5CG4202696:/mnt/c/Users/saleemmehmoo/riscv-internship/vsdsquadron-mini-core/task1$ ./task1_demo
Starting firmware application
GPIO 5 initialized as OUTPUT
GPIO 3 initialized as INPUT
GPIO 5 write value: 1
GPIO 3 read value
Button state: 1
GPIO 5 write value: 0
```

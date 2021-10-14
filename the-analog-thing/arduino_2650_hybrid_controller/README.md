# Simple Arduino 2650 based hybrid controller
The directory contains all code required for a simple Arduino MEGA 2650 based
hybrid controller for [THE ANALOG THING](https://the-analog-thing.org).

This little analog computer features a HYBRID connector which allows an 
external computer or microcontroller to take control of the analog computer. 
It also has four level shifters so that up to four analog values can be 
read out by an attached digital computer. 

## Connections required
The required hardware consists of only a few wires to connect the HYBRID
port with the attached microcontroller:

|HYBRID pin|description       |destination|
|----------|------------------|-----------|
|2         |analog x output   |AnalogIn 0 |
|4         |analog y output   |AnalogIn 1 |
|6         |analog z output   |AnalogIn 2 |
|8         |analog u output   |AnalogIn 3 |
|9+10      |GND               |GND        |
|13        |enable hybrid mode|D2         |
|14        |MOP               |D3         |
|16        |MIC               |D4         |

## Software
The source for this little hybrid controller can be found in 
[simple_hybrid_controller.ino](simple_hybrid_controller/simple_hybrid_controller.ino). 
It requires two additional libraries, namely TimerThree and Timer Five to 
take care of the various timing issues in a hybrid computer. These libraries
must be installed for your Arduino IDE before compiling the source.

## Commands

* arm: Arm the data logger. Data logging can be performed during a single
       run (see below) and will automatically start when the logger has been
       armed before starting the single run. After completing the single 
       run, the logger is disarmed.
* channels=value: Set the number of analog channels to be logged. The value
       can range between 1 and 4.
* disable: Disable the hybrid controller. This is the normal startup mode;
           THE ANALOG THING will work as a standalone analog computer when
           the hybrid port is disabled.
* enable: Enable the hybrid controller. In this case, THE ANALOG THING is
          is under control of the attached microcontroller and the mode 
          switch is disabled.
* halt: Put THE ANALOG THING into HALT-mode.
* help: Print a short help text.
* ic: Set THE ANALOG THING to IC-mode.
* ictime=value: Set the IC-time to value milliseconds. This is necessary
                  for any single or repetitive run. Typically the duration 
                  of IC-mode is a few milliseconds. (If there are any 
                  integrators in an analog program with their time scale 
                  factor set to SLOW, the IC-time should be high enough so 
                  that these integrators can settle to the selected initial
                  condition.)
* interval=value: Set the sampling interval for the data logger to 
                    value milliseconds.
* op: Set THE ANALOG THING to OP-mode.
* optime=value: Set the OP-time to value milliseconds. 
* rep: Start repetitive operation. THE ANALOG THING will from now on cycle
       between IC and OP with the respective times set by the ictime and 
       optime commands. Repetitive operation is ended when the computer 
       is explicitly set into IC-, OP- or HALT-mode.
* run: Start a single run with the IC- and OP-phase duration determined by 
       the times set with the ictime and optime commands. Only during such
       a single run data logging is possible.
* status: Return status information.

## Typical operation
A typical sequence of commands to run an analog program once while collecting
data looks like this:

```
ictime=50
optime=3000
interval=5
channels=1
arm
enable
run
```

This will print the AD converted values during the single run. These can then
be copied to a file (by copy/paste) and plotted with gnuplot or the like.

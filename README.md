# Motor timer
Timer for controling DC brushed motor on uncontroled aircraft model

## Features:
* Time of thrust can be set in range 5-70s by onboard trimer.
* Thrust can be set in range 0-100% by onboard trimer.
* 5s long ramp of motor turnning off.
* Module shall work with voltage 3.0-5.0V with load up to 3A. (Tested with one Li-Po cell and DC mottor consuming around 1A)

## How to use it:
Set desired time and throttle by trimmers. Turning clockwise means more throttle / longer time.
1) Connect batery.
2) Push the button. At this moment moduler reads values from trimmers, turn on the motor and start count down.
3) Now mottor is running and can be stopped by:
 a) Finishing of count down. (Soft turn off whith ramp down)
 b) Pushing the button. (Turn off wihout any ramp)

Alway power module from accumulator. Do not use laboratory power supply, wall chargers, etc...

AC Power Meters
===============

This repository contains code and hardware for a set of AC power meters.

ACme++
------

ACme++ is an updated version of the [ACme](http://acme.eecs.berkeley.edu) device.
The major change is a switch from the Epic platform (MSP430F1611 and CC2420) to
the CC2538 system on chip. Other changes include putting the relay on the phase
wire (instead of neutral) and a smaller AC-DC power supply.

While ACme++ is a wireless power meter in its own right, it was designed to be 
a prototype for the ultra-space-constrained PowerCube device.

PowerCube
---------

PowerCube is a one cubic inch power meter. It is designed to unobtrusively fit
on a power strip. This requires using six PCBs arranged in a cube with very
strategically placed components.


Software
--------

Software for the ACme++ is written on top of ContikiOS.



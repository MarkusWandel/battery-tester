# NiMH AA Battery Tester Manual
# Usage
# How it works
## Trickle Charging
A 1-hour fast charger has to terminate the charge when the battery is full, else the charge current is all turned into heat and the battery is damaged.  Charge termination for NiMH batteries is tricky.  Thus it can't be assumed that the 1-hour charger will completely charge the battery.

Trickle charging is defined as a reate of C/10 or less, where C is the capacity of the battery.  So for example a 2800mAh battery can be trickle charged at 280mA.  Trickle charging does not need careful charge termination, though a battery should not be trickle charged indefinitely.  Most cheap overnight chargers work this way.

This battery tester includes a trickle topup, on the assumption that a fast charged battery isn't 100% full.  The trickle current is around 160mA (5V through a 22 ohm resistor) for batteries down to 1600mAh capacity.  The charge time is selectable.  After the charge is complete, the tester will either stop or commence discharge testing, depending on mode.

# Construction
## Materials
The list of electronic materials is at the end of the [Schematic](../schematic/battery-tester.pdf).
Other materials used to build the device as shown in the photo were:

- 12x18cm proto board with copper pads
- about 80cm of 14-gauge copper wire (obtain by stripping the insulated part of normal house wiring - the bare coppper earth wire is a lesser gauge)
- about 30cm of foam double-sided tape
- some insulated hookup wire (from a stripped twisted pair cable such as used for ethernet)
- wirewrap wire
- solder
- electrical tape

## Tools
Tools to construct the device as shown were
- Good quality temperature regulated soldering iron with a reasonably fine tip
- Dremel tool with a 1/16" milling bit
- Needlenose pliars
- Wire cutter
- Wire stripper
- Scissors
- Tweezers

## Method
Everyone has their own preferred style for a one-off project.  You may enjoy designing a custom printed circuit board, or you may just assemble it using breadboards and jumper wires.  The following just shows how I made mine.

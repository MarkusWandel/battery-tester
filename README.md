# NiMH AA Battery Tester
## Intro
We have way too many rechargeable batteries around the house.  And while these should be carefully used in matched sets, labeled and tracked, none of this happens in real life.  You just grab some of the same brand, slam them into the 1-hour charger and get the impatient child's toy going again.

Result: They should work better than this.  Are they working anywhere near the advertised capacity any more?  Should they be thrown out and replaced by new ones?  Which ones are the closest match to combine into a set?

Let's build a tester.
## Test Method
Simply discharge a fully charged battery into a load resistor, and measure how long it lasts until it crosses a low voltage threshold.  This tester additionally graphs the voltage versus time on a bitmap LCD and tallies up milliamp hours and watt hours - once a second computing the current from the voltage reading, and integrating that.  The stop threshold is 1.0 Volt, which based on observation is well after the discharge curve becomes steep, i.e. the battery is exhausted.  The load resistance is 3.3 ohms which gives a reasonable test length (overnight) for a high capacity cell in good condition.  When the stop threshold is reached, the load resistor is disconnected.
## Trickle Charging
A 1-hour fast charger has to terminate the charge when the battery is full, else the charge current is all turned into heat and the battery is damaged.  Charge termination for NiMH batteries is tricky.  Thus it can't be assumed that the 1-hour charger will completely charge the battery.

Trickle charging is defined as a reate of C/10 or less, where C is the capacity of the battery.  So for example a 2800mAh battery can be trickle charged at 280mA.  Trickle charging does not need careful charge termination, though a battery should not be trickle charged indefinitely.  Most cheap overnight chargers work this way.

This battery tester includes a trickle topup, on the assumption that a fast charged battery isn't 100% full.  The trickle current is around 160mA (5V through a 22 ohm resistor) for batteries down to 1600mAh capacity.  The charge time is selectable.  After the charge is complete, the tester will either stop or commence discharge testing, depending on mode.

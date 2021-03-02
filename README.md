# NiMH AA Battery Tester
[Manual](doc/manual.md) [Schematic](doc/schematic.pdf) [Source Code](src/battery-tester.ino)
## Intro
We have way too many rechargeable batteries around the house.  And while these should be carefully used in matched sets, labeled and tracked, none of this happens in real life.  You just grab some of the same brand, slam them into the 1-hour charger and get the impatient child's toy going again.

Result: They should work better than this.  Are they working anywhere near the advertised capacity any more?  Should they be thrown out and replaced by new ones?  Which ones are the closest match to combine into a set?

Let's build a tester to find out.
## Test Method
After an optional trickle charge topup, discharge a fully charged battery into a load resistor, and measure how long it lasts until it crosses a low voltage threshold.  This tester additionally graphs the voltage versus time on a bitmap LCD and tallies up milliamp hours and watt hours - once a second computing the current from the voltage reading, and integrating that.  The stop threshold is 1.0 Volt, which based on observation is well after the discharge curve becomes steep, i.e. the battery is exhausted.  The load resistance is 3.3 ohms which gives a reasonable test length (overnight) for a high capacity cell in good condition.  When the stop threshold is reached, the load resistor is disconnected.

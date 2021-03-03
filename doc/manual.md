# NiMH AA Battery Tester Manual
# Usage
They say a picture is worth a thousand words, so here's one:

![Operation Flowchart](pix/flowchart.jpg)

- **Do not power device using USB and wall adapter at the same time.**  This could cause problems since both would drive the same 5V rail.  The device will work on USB power alone as long as it does not go into charge mode.  This is enough to test firmware changes.
- **Do not insert batteries while the device is powered up.**  If you were to get one backwards, it could burn out the Arduino, since backward battery test is only done right after powerup.
- A discharged battery (less than 1.0V into 3.3 ohm load) will show the same as an empty socket
- The sequence of update of the six channels changes every horizontal pixel (two minutes).  This is so each colour gets an equal chance when several graph occupy the same pixel.  The last one to be drawn wins.  Two lines exactly on top of each other should show alternating colour pixels.
- Voltages higher than about 1.42V aren't shown on the graph, but still measured and tallied correctly.  This can happen when discharging non-rechargeable alkaline AA cells (see below).
- Times higher than 8 hours aren't shown on the graph, but still measured and tallied correctly.  This could theoretically happen with a high-enough capacity cell.

# What if you discharge an Alkaline Cell?

These are the discharge traces of two different brand private label alkaline AAs.  The traces look pretty similar.

![Alkaline Battery Discharge](pix/alkaline.jpg)

Conclusions:

- It is practical to (destructively) compare cheap vs. expensive alkaline AA's with this tester
- An alkaline battery has too high an ESR (equivalent series resistance) to efficiently yield its energy into 3.3 ohms.

The milliamp hour tallying code wasn't as accurate at this point and watt hours weren't counted.

According to [Wikipedia](https://en.wikipedia.org/wiki/Alkaline_battery#Voltage) at significant load an alkaline cell hits 1.0V with still about 20% energy remaining, and [elsewhere on Wikipedia](https://en.wikipedia.org/wiki/AA_battery#Comparison) it says that the rated capacity of an alkaline AA, 1800-2850mAh (presumably cheap private label ones are at the low end) is yielded at a 50mA discharge rate.  NiMH rechargeables work better for high loads.

# How it works
## Discharge Circuit
This is the important circuit of the tester.

![Discharge Circuit](pix/discharge.jpg)

Every connection in this circuit except as noted is made of very heavy (14 gauge copper) wire to eliminate any change of stray resistance throwing off the measurement.  If there is extra resistance to either terminal of the battery holder, including in the relay, the Arduino will see a lower voltage than at the battery.

If there is extra resistance in the ground connection to the Arduino, then the power supply current of it and the LCD will slightly raise the GND potential that the analog-digital converter references to, causing a reading that is too low.

The entire discharge loop needs to be as low resistance as possible, so the load resistance on the battery really is 3.3 ohms and not more.  The firmware in the Arduino uses 3.3 ohms when converting voltage to current for measuring milliamp hours and watt hours.

The only connection where resistance doesn't matter much, and that can be done with a thin wire, is the labeled one.  This is because the analog input pins on the Arduino draw only an immeasurably small amount of current.

A NiMH cell under load has a nominal output voltage of 1.2V.  The six discharge resistors burn 6\*1.2\*1.2/3.3 = 2.6W while doing the discharge test.  The 0.44W per resistor is far below the rated capacity of the 5W wirewound resistors, but still, they get almost too hot to touch over time.
## Reference Voltage for A/D conversion
The Arduino's analog-digital converter outputs a value of 0 if the analog input is at ground level, and a value of 1023 if the analog input is at the analog reference level.  Without anything connected to the AREF pin this defaults to the supply voltage.  However, the supply voltage isn't necessarily very precise, only having to be within +/- 10% of 5V.  For any kind of exact measurement, a more precisely regulated analog reference voltage is required.

![Analog Reference Circuit](pix/aref.jpg)

A TL431 is used as it is commonly found in switching power supplies.  As connected here, it produces 2.5V.  The analog readings were still noisy with this configuration in a breadboard test, so the 10 microfarad capacitor was added.

The analog resolution is about 2.44mV per A/D converter step.  This is adequate for this application; however if a lower reference voltage were used (about 1.6V) the voltage resolution would be higher.
## Trickle Charging
A 1-hour fast charger has to terminate the charge when the battery is full, else the charge current is all turned into heat and the battery is damaged.  Charge termination for NiMH batteries is tricky.  Thus the battery won't necessarily be 100% charged.

Trickle charging is defined as a rate of C/10 or less, where C is the capacity of the battery.  So for example a 2800mAh battery can be trickle charged at 280mA.  Trickle charging does not need careful charge termination and can be left on overnight.  Most cheap overnight chargers work this way.

This battery tester includes a trickle topup so 100% charged batteries can be tested.

![Charging Circuit](pix/charge.jpg)

The charge relay is shown in the "active" position and the battery relay in the "inactive" position.  Current flows through the diode and the 22 ohm resistor to charge the battery.  The diode exists because otherwise all batteries not being discharged would be connected in parallel.  With the diode, current can only flow "into" the battery, not out of it.

With the 5 volt supply minus approximately 1.5 volts on a battery being charged, the current is 3.5V/22ohm = 159mA.  This is OK for batteries down to 1590mAh.  Smaller ones can still be topped up, just not for too many hours.

The charging circuitry is wasteful in that (5-1.5)/5 = 70% of the charge energy is just turned into heat by the 22 ohm resistor, for about (5-1.5V)\*0.159A = 0.56 watts per resistor.  This is far below the rated capacity of the 5W wirewound resistors, however they still get almost too hot to touch at 6\*0.56 = 3.36W of heat being generated continuously.
## Reverse Polarity Detection
To obtain precise readings, the batteries are connected directly to the Arduino's analog input pins when in discharge mode.  If this were to occur with a battery accidentally inserted backwards, the analog input pin would instantly burn out.

Given that the battery holders are confusingly marked (alternating up and down facing battery symbols), the following circuit is employed to guard against accidental damage.

![Polarity Sensing Circuit](pix/polarity.jpg)

When the electronics are powered up, resistor R13 tries to turn on transistor Q1, which then pulls the Arduino's input pin low.  This indicates "OK".

Any installed batteries will draw current away from the transistor's base connection through resistor R14, to try to turn the transistor off.  The exact turnoff voltage depends on the ratio of  R13 to R14.  100 ohms was experimentally determined to turn off the transistor at about -16 millivolts on the lowest-voltage battery.

R14 can be omitted, i.e. replaced by a direct connection, if you don't mind the tester saying "Battery Backwards!" and refusing to operate when a completely dead (0 volts) battery is installed.
## I2C Voltage Conversion
The Arduino runs on 5V, whereas the LCD runs at 3.3V.  While it is possible to directly wire SPI between incompatbile voltage levels, a programming error (drive the Arduino pin to logic high as opposed to letting it float high via a 3.3 ohm pullup resistor) could damage the LCD this way.

A simple MOSFET circuit described on [This web site](https://www.hobbytronics.co.uk/mosfet-voltage-level-converter) shows how to convert SPI logic levels.  Four of these are available on a simple
module which is used here.
## Sources of Inaccuracy
This is a device that measures something.  As such it should be built to instrument grade.  It isn't.  The following sections discuss sources of inaccuracy.
### Stray Resistance
Heavy (overkill at 14 gauge) wire is used where stray resistance would throw the measurement off.

The battery holders are of a type that has very sturdy metal contact tabs.  The screw terminals on the relay module are tightened well.
### Load Resistor Accuracy
The load resistors are supposed to be 3.3 ohms.  How close to that are they?  The Arduino needs this value for voltage-current conversions.  It's hard to directly measure such a low resistance with a multimeter.  Instead, two multimeters and a power supply can be used.  One meter measures the current, the other the voltage.

The device, as built, shows 3.3 ohms +/- about 5% at the battery terminals in discharge mode.

I did not test whether the resistors heating up significantly affects their resistance.
### Consistency between Channels
Even more important than the accuracy of the load resistors is that the channels all give consistent readings.  Otherwise a battery might appear stronger than another, just because it is in the more favourable position.

Aside from ensuring that all the analog input pins on the Arduino read the same for the same voltage, which I did not check, and really controlling stray resistance in the wiring, the main task was to ensure the resistors are all the same.  This was done by connecting all six of them in series to a power supply before installation, and measuring the voltage across each with a multimeter.  To the extent the multimeter could tell (3 signifcant digits) they all had the same voltage drop.
### Analog Reference Accuracy
The TL431 device is not instrument grade.  It works well enough for this application.
### Conversion of A/D Converter reading to Millivolts
In the firmware, the following conversion is used on the sum of 1000 analog readings:

&emsp; millivolts = (100\*adc+20165) / 40330

The divisor of 403.3 was experimentally determined to give the best accuracy.  The conversion is done like this to make integer arithmetic work.  Doing "10\*adc/4033" takes care of the decimal place, and adding half the divisor takes care of rounding, only that wouldn't be an integer, so I multiply by 10 again just so the rounding addend is an integer.  Multiplying by 2 would have been sufficient, but this is more readable.

This was calibrated by not yet connecting one of the 22 ohm load resistors, and connecting a potentiometer between ground, +5V and the positive battery terminal.  This allowed me to dial in various voltages and observe the millivolt variable continously (via a print loop to the serial monitor).

In practice, checking battery voltage with a multimeter and comparing to the voltage shown on the device (with two decimal places) I find the accuracy to be within +/- 1%.
### Electrical and A/D Converter Noise
The main method to deal with this is to simply read each voltage 1000 times and average.  The 1000 readings are interleaved between channels to space the readings out evenly over most of a second.  It was not tested whether this (having the A/D converter input constantly switching channels) is, in fact, less noisy than doing each batch of 1000 reads separately.

The 10 microfarad filter capacitor was necessary in my breadboard test. I just included it in the build without testing whether still necessary there with much better grounding.
### Fixed-Point Arithmetic
The firmware uses only integer, or rather fixed point arithmetic.  If I want volts with two decimal places, I call it "centivolts" and compute accordingly.  Whenever something needs dividing by something else, it is always on this principle:

&emsp; Result = (Input + Divisor/2) / Divisor

Where both Input and Divisor are scaled up enough to make Divisor/2 an integer.  This results in correct rounding.

In general, operations that lose precision are avoided for the milliamp hour and watt hour accumulators.  Thus dividing by 3.3 for the load resistance is only done just before displaying the results; the continous accumulation only involves adds and multiplies.  The exception to this is the initial analog reading to millivolt conversion; it would be too confusing to defer this.
# Construction
## Materials
The list of electronic materials is at the end of the [Schematic](../schematic/battery-tester.pdf).
Other materials used to build the device as shown in the photos were:

- Prototyping board, 18x12cm with copper pads
- About 80cm of 14-gauge copper wire (from a stripped scrap of house wiring cable)
- About 30cm of foam double-sided adhesive tape
- Electrical tape
- Insulated hookup wire (from a stripped twisted pair cable such as Cat5)
- Wirewrap wire (optional)
- Solder

## Tools
Tools to construct the device as shown were

- Good quality temperature regulated soldering iron with a reasonably fine tip
- Solder sucker or solder wick braid to fix soldering errors
- Dremel tool with a 1/16" milling bit (optional - otherwise just a drill)
- Wirewrap gun (optional)
- Needlenose pliers
- Wire cutter
- Wire stripper
- Scissors
- Tweezers

## Method
Everyone has their own preferred style for a one-off project.  You may enjoy designing a custom printed circuit board, or you may just assemble it using breadboards and jumper wires.  The following just shows how I made mine.

It is advisable to test the high pin count parts on a breadboard before soldering them in.  Run the firmware and make sure the LCD displays the config screen.  Maybe test the analog inputs.

Battery holders mounted.  I milled slots into the proto board using the dremel tool, bent the tabs straight and passed them through.  Round holes made with an ordinary drill would work too, just not look as pretty.

Considering the effort to control stray resistance, the very heavy duty tabs (take a real effort to bend and real force to insert the batteries) are perfect.

Note the very heavy ground wire.  I ran it around in a loop to further reduce the resistance between the load resistors and the negative battery terminals.

![Assembly Photo 1](pix/assembly1.jpg)

Next, cover the part where the relay modules will go with two layers of electrical tape.

![Assembly Photo 2](pix/assembly2.jpg)

Mount the relay banks (one bank of eight would be better) using two layers of double-sided adhesive foam tape.  They won't come off due to the stiffness of the heavy wires that will be added.  Here we already have the connections to the positive battery terminals.  They should be high enough that the stuff sticking out of the bottom won't make contact with the copper on the prototyping board.

![Assembly Photo 3](pix/assembly3.jpg)

The 3.3 ohm load resistors are mounted.

![Assembly Photo 4](pix/assembly4.jpg)

And connected to the "normally open" terminals on the relays at one end, and the ground wire at the other.

![Assembly Photo 5](pix/assembly5.jpg)

The 22 ohm charge resistors are mounted.

![Assembly Photo 6](pix/assembly6.jpg)

And connected to the "normally closed" relay terminals at one end, and the charge diodes at the other end.  The common end of the charge diodes goes to the "common" terminal on the charge relay.  Heavy hookup wire is used, but not the 14 gauge stuff.  Stray resistance here does not affect the measurement.

![Assembly Photo 7](pix/assembly7.jpg)

What could affect the measurement is the ground connection to the Arduino.  Both ground pins are used.  No need for heavy wire here, because the wire runs are very short.

![Assembly Photo 8](pix/assembly8.jpg)

Top side component view.  This is after the first overnight test run.  The reverse polarity circuit is still missing.  The components above the Arduino are the 2.5V analog reference.

![Assembly Photo 9](pix/assembly9.jpg)

Bottom wiring.  Noncritical connections are wirewrapped since I happen to have wirewrap stuff.

The power cord does not have a connector.  If the wrong kind of power adapter were to be plugged in the Arduino would be burned out, since the power supply's regulated 5V powers the AVR328 CPU directly.  So the power adapter cannot be removed.

![Assembly Photo 10](pix/assembly10.jpg)

## What I Would Do Differently Next Time
The power resistors get fairly hot.  They are in two groups; first all the charge resistors and then all the discharge ones.  It would make more sense to interleave them to spread the heat out more, since only one or the other is active at a time.  Also the leads could be left a bit longer to elevate them off the prototyping board for better air circulation.

Maybe use a bigger display.  The very cheap 1.3" one clearly communicates its information but at my age, at least, it's squinty.

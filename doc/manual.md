# NiMH AA Battery Tester Manual
# Usage
# How it works
## Discharge Circuit
This is the important circuit of the tester.

![Discharge Circuit](pix/discharge.jpg)

Every connection in this circuit except as noted is made of very heavy (14 gauge copper) wire to eliminate any change of stray resistance throwing off the measurement.  If there is extra resistance to either terminal of the battery holder, including in the relay, the Arduino will see a lower voltage than at the battery.

If there is extra resistance in the ground connection to the Arduino, then the power supply current of it and the LCD will slightly raise the GND potential that the analog-digital converter references to, causing a reading that is too low.

The entire discharge loop needs to be as low resistance as possible, so the load resistance on the battery really is 3.3 ohms and not more.  The firmware in the Arduino uses 3.3 ohms when converting voltage to current for measuring milliamp hours and watt hours.

The only connection where resistance doesn't matter much, and can be done with a thin wire, is the labeled one.  This is because the analog input pins on the Arduino draw only an immeasurably small amount of current.

A NiMH cell under load has a nominal output voltage of 1.2V.  The six discharge resistors burn 6*1.2*1.2/3.3 = 2.6W while doing the discharge test.  The 0.44W per resistor is far below the rated capacity of the 5W wirewound resistors, but still, they get almost too hot to touch over time.
## Reference Voltage for A/D conversion
The Arduino's analog-digital converter outputs a value of 0 if the analog input is at ground level, and a value of 1023 if the analog input is at the analog reference level.  Without anything connected to the AREF pin this defaults to the supply voltage.  However, the supply voltage isn't necessarily very precise, only having to be within +/- 10% of 5V.  For any kind of exact measurement, a more precisely regulated analog reference voltage is required.

![Analog Reference Circuit](pix/aref.jpg)

The device chosen is a TL431.  In the configuration here, it produces 2.5V.  The 2.2K ohm resistor limits the current through the device.  The analog readings were still noisy with this configuration, so the 10 microfarad capacitor was added.

Ideally the reference voltage should be just higher than the highest voltage expected to be measured; in this case, about 1.5V.  However the TL431 doesn't go any lower than 2.5V.  Thus part of the analog/digital converter resolution is wasted; a typical discharge voltage of a NiMH cell of 1.2 volts will only produce 1.2/2.5*1023 = approximately 491 on the analog-digital converter output, giving a resolution of 1.2V/491 = 2.44mV.  Two resistors as a voltage divider between the TL431 and the filter capacitor could be added to address this.

In actual fact, the reading resolution is higher, because each reading is taken 1000 times and averaged to smooth out analog-digital conversion noise.  For "inbetween steps" values it is likely that some are at the higher step and some at the lower step, and the ratio of these readings will determine the final result.
## Trickle Charging
A 1-hour fast charger has to terminate the charge when the battery is full, else the charge current is all turned into heat and the battery is damaged.  Charge termination for NiMH batteries is tricky.  Thus it can't be assumed that the 1-hour charger will completely charge the battery.

Trickle charging is defined as a rate of C/10 or less, where C is the capacity of the battery.  So for example a 2800mAh battery can be trickle charged at 280mA.  Trickle charging does not need careful charge termination, though a battery should not be trickle charged indefinitely.  Most cheap overnight chargers work this way.

This battery tester includes a trickle topup, on the assumption that a fast charged battery isn't 100% full.

![Charging Circuit](pix/charge.jpg)

The charge relay is shown in the "active" position and the battery relay in the "inactive" position.  Current flows through the diode and the 22 ohm resistor to charge the battery.  The diode exists because otherwise all batteries not being discharged would be connected in parallel.  With the diode, current can only flow "into" the battery, not out of it.

The 22 ohm resistance is chosen so about 160mA of charging current flows: 5 volt supply minus approximately 1.5 volts on a battery being charged, current = 3.5V/22ohm = 159mA.  Since it is safe to trickle charge a battery at 1/10 its rated capacity, this is good for batteries down to 1590mAh.  As long as trickle charging isn't for too long, it can be used on smaller capacity batteries too, in my case, on some cheap 1350mAh ones.

The charging circuitry is wasteful in that (5-1.5)/5 = 70% of the charge energy is just turned into heat by the 22 ohm resistor, for about (5-1.5V)*0.159A = 0.56 watts per resistor.  This is far below the rated capacity of the 5W wirewound resistors, however they still get almost too hot to touch comfortably on my device at 6*0.56 = 3.36W of heat being generated continuously.
## Reverse Polarity Detection
To obtain precise readings, the batteries are connected directly to the Arduino's analog input pins when in discharge mode.  If this were to occur with a battery accidentally inserted backwards, the analog input pin would instantly burn out.

Modern electronics generally make it impossible for batteries to be inserted backwards, or have provisions against damage if they are.  And the battery holders used here have misleading markings, with one side showing a battery symbol with the positive end at the bottom.  So this circuit is employed:

![Polarity Sensing Circuit](pix/polarity.jpg)

When the electronics are powered up, resistor R13 tries to turn on transistor Q1, which then pulls the Arduino's input pin low.  This indicates "OK".

Any installed batteries will pull the transistor's base voltage lower via resistor R14, to try to turn the transistor off.  The exact turnoff voltage depends on the ratio of  R13 to R14.  100 ohms was experimentally determined to turn off the transistor, and therefore turn on the Arduino's input pin, if any battery has a voltage of about -16 millivolts or lower.  Because of the charge diodes, the lowest-voltage battery (i.e. the backward one) "wins" over the other batteries.

It is important that the negative voltage not reach the Arduino even if the circuit is depowered and the transistor can't turn on.  This is the case because of the built-in diode action of the transistor's emitter-base junction.  In other words, a negative voltage coming in via R14 can't "pull" on the Arduino's input pin through the transistor.
R14 can be omitted, i.e. replaced by a direct connection, if you don't mind the tester saying "Battery Backwards!" and refusing to operate when a completely dead battery (0 volts) battery is installed.  Such a battery could well be backwards and would, in that case, be damaged by trying to charge it.  In my device, with R14 at 100 ohms, a completely dead, 0 volts battery will be allowed.
## I2C Voltage Conversion
The Arduino runs on 5V, whereas the LCD runs at 3.3V.  SPI is an "open collector" protocol, meaning, the logic high level is determined by a pullup resistor.  Thus it is possible, with discipline, to connect the Arduino directly to the LCD.  However, if the Arduino's internal pullup resistors are used, they will pull the logic level above 3.3V.  External pullup resistors to 3.3V would be needed.   Even then, if the Arduino actively drives the signal high, the display's maximum input voltage level will be exceeded.  Rather than deal with all that, a simple MOSFET voltage converter is used.
[This web site](https://www.hobbytronics.co.uk/mosfet-voltage-level-converter) shows how these work.  Instead of wiring it up from scratch, a cheap 4-channel converter module is used.  The Arduino's 3.3V output is sufficient to power the converter and the LCD including backlight.
## Sources of Inaccuracy
This is a device that measures something.  As such it should be built to instrument grade.  It isn't.  The relays are cheap generic ones, and the Arduino's analog-digital converter, while pretty accurate, is no multimeter.  The following sections show sources of inaccuracy and how they are dealt with.
### Stray Resistance
As mentioned earlier, heavy gauge wire is used where stray resistance would throw the measurement off.  The 14 gauge wire is overkill, resulting in fractions of a milli-ohm of maximal interconnect resistance, but I had that gauge of wire available.  Regular thin hookup wire would be insufficient.

The battery holders are of a type that has very sturdy metal contact tabs.
### Load Resistor Accuracy
The load resistors are supposed to be 3.3 ohms.  The Arduino needs this value for voltage-current convresions.  Are the resistors really 3.3 ohms?  It's hard to measure such a low resistance with hobbyist gear.

Instead, on the finished device, I powered one of the battery terminals from an adjustable power supply, with multimeters in series (to measure current) and parallel to the battery contacts (to measure voltage).  To the extent the multimeters' resolution allowed me to measure it, I saw a total resistance of 3.3 ohms plus/minus about 5%.

I did not test whether the resistors heating up significantly affects their resistance.
### Differences between Channels
Even more important than the accuracy of the load resistors is that the channels all give consistent readings.  Otherwise a battery might appear stronger than another, just because it is in the more favourable battery holder.

Aside from ensuring that all the analog input pins on the Arduino read the same for the same voltage, which I did not check, and really controlling stray resistance in the wiring, the main task was to ensure the resistors are all the same.  This was done by connecting all six of them in series to a power supply before installation, and measuring the voltage across each with a multimeter.  To the extent the multimeter could tell (3 signifcant digits) they all had the same voltage drop.
### Analog Reference Accuracy
The TL431 device is not instrument grade.  However in this application it produces a sufficiently accurate 2.5V reference voltage independent of the exact power supply voltage.  A better voltage reference would be overkill.
### Conversion of ADC reading to Millivolts
In the firmware, the following conversion is used on the sum of 1000 analog readings:

&emsp; millivolts = (100*adc+20165) / 40330

The divisor of 403.3 was experimentally determined to give the best accuracy.  The conversion is done like this to make integer arithmetic work.  Doing "10*adc/4033" takes care of the decimal place, and adding half the divisor takes care of rounding, only that wouldn't be an integer, so I multiply by 10 again just so the rounding addend is an integer.  Multiplying by 2 would have been sufficient, but this is more readable.

This was "calibrated" by not yet connecting one of the 22 ohm load resistors, and connecting a potentiometer between ground, +5V and the positive battery terminal.  This allowed me to dial in various voltages and observe the millivolt variable continously (via a print loop to the serial monitor).

In practice, checking battery voltage with a multimeter and comparing to the voltage shown on the device (with two decimal places) I find the accuracy to be within +/- 1%.
### Electrical and A/D Converter Noise
The main method to deal with this is to simply read each voltage 1000 times and average.  The 1000 readings are interleaved between channels to space the readings out evenly over most of a second.  It was not determined whether this (having the A/D converter input constantly switching channels) is, in fact, less noisy than doing each batch of 1000 reads separately.

The analog reference voltage needed the 10 microfarad filter capacitor in my breadboard test.  I didn't try whether it was still necessary in the final construction with much better ground and shorter interconnects.
### Fixed-Point Arithmetic
The firmware uses only integer, or rather fixed point arithmetic.  If I want volts with two decimal places, I call it "centivolts" and compute accordingly.  Whenever something needs dividing by something else, it is always on this principle:

&emsp; Result = (Input + Divisor/2) / Divisor

Where both Input and Divisor are scaled up enough to make Divisor/2 an integer.  This results in no rounding loss.

In general, operations that lose precision are avoided for the milliamp hour and watt hour accumulators.  Thus dividing by 3.3 to get the current is only done just before displaying the results; the continous accumulation only involves adds and mltiplies.  The exception to this is the initial analog reading to millivolt conversion; it would be too confusing to defer this.
# Construction
## Materials
The list of electronic materials is at the end of the [Schematic](../schematic/battery-tester.pdf).
Other materials used to build the device as shown in the photos were:

- 18x12cm prototyping board with copper pads
- about 80cm of 14-gauge copper wire (obtain by stripping the insulated part of normal house wiring - the bare coppper earth wire is a lesser gauge)
- about 30cm of foam double-sided adhesive tape
- electrical tape
- some insulated hookup wire (from a stripped twisted pair cable such as used for ethernet)
- wirewrap wire
- solder

## Tools
Tools to construct the device as shown were
- Good quality temperature regulated soldering iron with a reasonably fine tip
- Dremel tool with a 1/16" milling bit
- Wirewrap gun
- Needlenose pliars
- Wire cutter
- Wire stripper
- Scissors
- Tweezers

## Method
Everyone has their own preferred style for a one-off project.  You may enjoy designing a custom printed circuit board, or you may just assemble it using breadboards and jumper wires.  The following just shows how I made mine.

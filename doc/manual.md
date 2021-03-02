# NiMH AA Battery Tester Manual
# Usage
# How it works
## Discharge Circuit
This is the important circuit of the tester.

![Discharge Circuit](pix/discharge.jpg)

Every connection in this circuit is made of very heavy (14 gauge copper) wire to eliminate any change of stray resistance throwing off the measurement.  If there is extra resistance to either terminal of the battery holder, including in the relay, the Arduino will see a lower voltage than at the battery.

If there is extra resistance in the ground connection to the Arduino, then the power supply current of it and the LCD will slightly raise the GND potential that the analog-digital converter references to, causing a reading that is too low.

The entire discharge loop needs to be as low resistance as possible, so the load on the battery really is 3.3 ohms and not more.  The firmware in the Arduino assumes 3.3 ohms here when converting voltage to current for measuring milliamp hours and watt hours.

The only connection where resistance doesn't matter much, and can be done with a thin wire, is the labeled one.  This is because the analog input pins on the Arduino draw only an immeasurably small amount of current.
## Reference Voltage for A/D conversion
The Arduino's analog-digital converter outputs a value of 0 if the analog input is at ground level, and a value of 1023 if the analog input is at the analog reference level.  Without anything connected to the AREF pin this defaults to the supply voltage.  However, the supply voltage isn't necessarily very precise, only having to be within +/- 10% of 5V.  For any kind of exact measurement, a more precisely regulated analog reference voltage is required.

![Analog Reference Circuit](pix/aref.jpg)

The device chosen is a TL431.  In the configuration here, it produces 2.5V.  The 2.2K ohm resistor limits the current through the device.  The analog readings were still noisy with this configuration, so the 10 microfarad capacitor was added.

Ideally the reference voltage should be just higher than the highest voltage expected to be measured; in this case, about 1.5V.  However the TL431 doesn't go any lower than 2.5V.  Thus part of the analog/digital converter resolution is wasted; a typical discharge voltage of a NiMH cell of 1.2 volts will only produce 1.2/2.5*1023 = approximately 491 on the analog-digital converter output, giving a resolution of 1.2V/491 = 2.44mV.  Two resistors as a voltage divider between the TL431 and the filter capacitor could be added to address this.
## Trickle Charging
A 1-hour fast charger has to terminate the charge when the battery is full, else the charge current is all turned into heat and the battery is damaged.  Charge termination for NiMH batteries is tricky.  Thus it can't be assumed that the 1-hour charger will completely charge the battery.

Trickle charging is defined as a reate of C/10 or less, where C is the capacity of the battery.  So for example a 2800mAh battery can be trickle charged at 280mA.  Trickle charging does not need careful charge termination, though a battery should not be trickle charged indefinitely.  Most cheap overnight chargers work this way.

This battery tester includes a trickle topup, on the assumption that a fast charged battery isn't 100% full.  The trickle current is around 160mA (5V through a 22 ohm resistor) for batteries down to 1600mAh capacity.  The charge time is selectable.  After the charge is complete, the tester will either stop or commence discharge testing, depending on mode.

![Charging Circuit](pix/charge.jpg)

The charge relay is shown in the "active" position and the battery relay in the "inactive" position.  Current flows through the diode and the 22 ohm resistor to charge the battery.  The diode exists because otherwise all batteries not being discharged would be connected in parallel.  With the diode, current can only flow "into" the battery, not out of it.

The 22 ohm resistance is chosen so about 160mA of charging current flows: 5 volt supply minus approximately 1.5 volts on a battery being charged, current = 3.5V/22ohm = 159mA.  Since it is safe to trickle charge a battery at 1/10 its rated capacity, this is good for batteries down to 1600mAh.  As long as trickle charging isn't for too long, it can be used on smaller capacity batteries too, in my case, on some cheap 1350mAh ones.
## Reverse Polarity Detection
To obtain precise readings, the batteries are connected directly to the Arduino's analog input pins when in discharge mode.  If this were to occur with a battery accidentally inserted backwards, the analog input pin would instantly burn out.

Modern electronics generally make it impossible for batteries to be inserted backwards, or have provisions against damage if they are.  And the battery holders used here have misleading markings, with one side showing a battery symbol with the positive end at the bottom.  So this circuit is employed:

![Polarity Sensing Circuit](pix/polarity.jpg)

When the electronics are powered up, resistor R13 tries to turn on transistor Q1, which then pulls the Arduino's input pin low.  This indicates "OK".

Any installed batteries will pull the transistor's base voltage lower via resistor R14, to turn the transistor off.  The exact turnoff voltage depends on the ratio of  R13 to R14.  100 ohms was experimentally determined to turn off the transistor, and therefore turn on the Arduino's input pin, if any battery has a voltage of about -16 millivolts or less.  Because of the charge diodes, the lowest-voltage battery (i.e. the backward one) "wins" over the other batteries.

It is important that the negative voltage not reach the Arduino even if the circuit is depowered and the transistor can't turn on.  This is the case because of the built-in diode action of the transistor's emitter-base junction.  In other words, a negative voltage coming in via R14 can't "pull" on the Arduino's input pin through the transistor.
R14 can be omitted, i.e. replaced by a direct connection, if you don't mind the tester saying "Battery Backwards!" and refusing to operate when a completely dead battery (0 volts) battery is installed.  Such a battery could well be backwards and would, in that case, be damaged by trying to charge it.  In my device, with R14 at 100 ohms, a completely dead, 0 volts battery will be allowed.
## I2C Voltage Conversion
The Arduino runs on 5V, whereas the LCD runs at 3.3V.  SPI is an "open collector" protocol, meaning, the logic high level is determined by a pullup resistor.  Thus it is possible, with discipline, to connect the Arduino directly to the LCD.  However, if the Arduino's internal pullup resistors are used, they will pull the logic level above 3.3V.  External pullup resistors to 3.3V would be needed.   Even then, if the Arduino actively drives the signal high, the display's maximum input voltage level will be exceeded.  Rather than deal with all that, a simple MOSFET voltage converter is used.
[This web site](https://www.hobbytronics.co.uk/mosfet-voltage-level-converter) shows how these work.  Instead of wiring it up from scratch, a cheap 4-channel converter module is used.  The Arduino's 3.3V output is sufficient to power the converter and the LCD including backlight.
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

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>             // Arduino SPI library
 
// ST7789 TFT module connections
#define TFT_CS     10
#define TFT_RST    8  // define reset pin, or set to -1 and connect to Arduino RESET pin
#define TFT_DC     9  // define data/command pin
 
// Initialize Adafruit ST7789 TFT library
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

#define VERSION_STR "v1.01"

// If the Arduino's clock is off, adjust the divisor here.  This setting compensates
// for one that runs 0.1% fast.
#define MILLIS_PER_SECOND 1001

//  ST77XX colours are in 5/6/5 bit R/G/B format.  Want a couple of shades of grey
//
//  00100 001000 00100
#define ST77XX_GREY 0x2104
//  10000 100000 10000
#define ST77XX_LGREY 0x8410

#define REVERSE_VOLT_PIN A0
#define CHARGE_RELAY_PIN A1
#define BUTTON_PIN 0

// The relay module has active low enables
#define RELAY_ON_VAL 0
#define RELAY_OFF_VAL 1

// How many seconds the config screen stays up (after last input) before the
// charge/discharge starts
#define CONFIG_TIMEOUT 15

struct {
   int adc_pin;     // Pin to sense the voltage
   int relay_pin;   // Pin to control the discharge relay
   int colour;      // Colour to use for this channel
} channels[6] = {
   {A2, 2, ST77XX_RED},
   {A3, 3, ST77XX_GREEN},
   {A4, 4, ST77XX_BLUE},
   {A5, 5, ST77XX_CYAN},
   {A6, 6, ST77XX_MAGENTA},
   {A7, 7, ST77XX_YELLOW}
};

// This function returns a valid result even if millis has wrapped around to zero
// since the "val" timestamp was captured.
unsigned long time_since(unsigned long val)
{
    return millis() - val;
}

int discharging[6];  // Tracks which channels are still discharging and have not stopped due to voltage cutoff

long millivolt_seconds[6];  // Accumulate in these units to avoid loss of precision
long millijoules[6];

// Ring buffers for averaging analog readings, to denoise them
//
// Note tha there is a hardcoded conversion in the discharge calculations
// that depends on the ring buffer size that does not use this parameter.

#define ANALOG_RINGBUF_SIZE 50   
int analog_ringbuf[ANALOG_RINGBUF_SIZE][6];
int analog_ringbuf_ptr;

unsigned long start_time;   // Tracks the start time of the different states

unsigned long charge_time;  // Selected charge time, in seconds

//-----------------------------------------------------------------------------
// This function draws the static parts of the discharge state screen

void draw_grid() {
  tft.fillScreen(ST77XX_BLACK);

  // Print the six square row labels

  tft.setTextSize(1);
  char label_str[5];
  label_str[1] = 0;
  for(int i=0; i<6; i++) {
     tft.setCursor(0, 10*i);
     tft.setTextColor(ST77XX_BLACK,channels[i].colour);
     label_str[0] = '1' + i;
     tft.print(label_str);
  }

  // 60 pixels of labels leaves 180 pixels of graph.  We want to graph from 1 to about 1.4 volt,
  // so the scale is 44 pixels per 1/10 volt.  The scale is divisible by 4 because dots are printed
  // at 25mv intervals

  // Voltage labels

  tft.setTextColor(ST77XX_LGREY);
  label_str[0] = '1';
  label_str[1] = '.';
  label_str[3] = 'V';
  label_str[4] = 0;
  for(int i=0;i<5;i++) {
     tft.drawLine(0, 239-44*i, 239, 239-44*i, ST77XX_GREY);  // 1/10 volt lines
     tft.setCursor(215, 230-44*i);
     label_str[2] = '0'+i;
     tft.print(label_str);
  }

  // Hour labels

  label_str[1] = 'h';
  label_str[2] = 0;
  for(int i=0;i<8;i++) {
     tft.drawLine(30*i, 60, 30*i, 239, ST77XX_GREY);         // hour lines
     tft.setCursor(30*i+1,230);
     label_str[0] = '0'+i;
     if(i-7) tft.print(label_str);  // don't print the last one because it conflicts with the volt label
  }

  // Dot grid at 1/40 volt and 20 minute intervals

  for(int i=0;i<24;i++) {
     for(int j=0;j<16;j++) {
        tft.drawPixel(10*i,239-11*j,ST77XX_GREY);
     }
  }
}

// The three UI states

#define S_CONFIG 1
#define S_CHARGE 2
#define S_DISCHARGE 3

int state;
unsigned long state_seconds;  // How long in the current state
int do_discharge;             // Set to 1 if discharge is to be done (rather than charge and stop)

// Predefined options for number of hours to charge

#define NUM_CH_HOUR_OPTIONS 8
int charge_hour_option;
int charge_hour_options[NUM_CH_HOUR_OPTIONS] = {0,1,2,4,8,12,16,20};

//-----------------------------------------------------------------------------
// This function prints the option screen.  It overprints, with trailing
// spaces if necessary, so the screen can be updated without clearing it
// every time.

void print_options() {
  tft.setTextSize(3);
  tft.setCursor(0,0);
  tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
  tft.print("Charge for");
  tft.setCursor(0,50);
  tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
  char msg[40];
  sprintf(msg,"%d hours ",charge_hour_options[charge_hour_option]);
  tft.print(msg);
  tft.setCursor(0,100);
  tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
  tft.print("then ");
  tft.setCursor(0,150);
  tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
  tft.print(do_discharge?"discharge":"stop     ");
}

//-----------------------------------------------------------------------------
// Before activating any relays, check for reverse voltage.  Since the Arduino's
// analog inputs are connected directly to the battery load resistors, activating
// a discharge relay with a reverse connected battery would immediately burn out
// the analog input pin.

void check_polarity() {
  if(digitalRead(REVERSE_VOLT_PIN)) {
     // Just indicate reverse voltage and stop.
     tft.fillScreen(ST77XX_RED);
     tft.setTextSize(3);
     tft.setCursor(0,100);
     tft.setTextColor(ST77XX_WHITE);
     tft.print("   Battery\n  Backwards!");

//     Uncomment this to test the backwards battery detector.  Once triggered, the screen
//     will turn green for OK, red for not OK
//
//     int rv = 1;
//     for(;;) {
//        int rn = digitalRead(REVERSE_VOLT_PIN);
//        if(rv != rn) {
//           tft.fillScreen(rn?ST77XX_RED:ST77XX_GREEN);
//           rv = rn;
//        }
//     }

     for(;;) { }  // freeze here
  }
}

//-----------------------------------------------------------------------------
// Debug printf for the serial monitor.  p(...) is about equivalent to
// fprintf(STDERR,...) when debugging in a Unix environment.  The actual
// serial operations are commented out by default.

void p(char *fmt, ... ) {
        char buf[128]; // resulting string limited to 128 chars
        va_list args;
        va_start (args, fmt );
        vsnprintf(buf, 128, fmt, args);
        va_end (args);
        //Serial.print(buf);
}

void setup() {
  //Serial.begin(115200);
  pinMode(CHARGE_RELAY_PIN,OUTPUT);
  digitalWrite(CHARGE_RELAY_PIN,RELAY_OFF_VAL);
  pinMode(BUTTON_PIN,INPUT_PULLUP);
  for(int i=0;i<6;i++) { 
     pinMode(channels[i].relay_pin,OUTPUT);
     pinMode(channels[i].adc_pin,INPUT);
     digitalWrite(channels[i].relay_pin,RELAY_OFF_VAL);
  }

  tft.init(240, 240, SPI_MODE2);    // Init ST7789 display 240x240 pixel
  tft.setRotation(2);
  tft.setTextWrap(false);

  pinMode(REVERSE_VOLT_PIN,INPUT_PULLUP);
  delay(200); // Wait 1/5 second to make sure everything is stable
  check_polarity();

  // Set up the "options" state
  
  tft.fillScreen(ST77XX_BLACK);
  print_options();
  state = S_CONFIG;
  do_discharge = 0;
  charge_hour_option = 0;
  start_time = millis();
}


// Software debounce and timer for the button (long vs. short press)
int prev_button_state = 0;
int button_state = 1;
int button_count = 0;
unsigned long button_down_time;

void loop() {
   int btn = digitalRead(BUTTON_PIN);
   if(btn != button_state) {
      button_state = btn;
      button_count = 3;
   } else if(button_count) {
      button_count--;
   } else {
      // Button has been in consistent state for 3 polls.
      if(button_state != prev_button_state) {
         if(button_state) {

            // Currently there is a button action only in the "config" state.  This is intentional,
            // nothing except disconnecting power (or a battery) should interrupt the charge or
            // or discharge operations to prevent accidental loss of data

            if(state == S_CONFIG) {
               if(time_since(button_down_time) > 600) {
                  do_discharge = !do_discharge;
               } else {
                  charge_hour_option = (charge_hour_option+1) % NUM_CH_HOUR_OPTIONS;
               }
               start_time = millis();  // Restart the config timeout at every input
               print_options();
            }
         } else {
            // button down.  Just remember the timestamp
            button_down_time = millis();
         }
      }
      prev_button_state = button_state;
   }

   unsigned long secs = time_since(start_time) / MILLIS_PER_SECOND;

   if(state == S_CONFIG) {
      if(secs < CONFIG_TIMEOUT) {
         // While in the config state, display in small print at the bottom
         // how many seconds are left until operation start

         if(secs != state_seconds) {
            tft.setTextSize(1);
            tft.setTextColor(ST77XX_GREY,ST77XX_BLACK);
            tft.setCursor(0,230);
            char msg[40];
            sprintf(msg,"Start in %d seconds",CONFIG_TIMEOUT-secs);
            tft.print(msg);
            tft.setCursor(210,230);
            tft.print(VERSION_STR);
            state_seconds = secs;
         }
      } else {
         // Config screen timed out, go to the charge state.
 
         state = S_CHARGE;
         start_time = millis();
         state_seconds = 0;
         charge_time = 3600L*charge_hour_options[charge_hour_option];

         // Only display the "charging" screen and set the relay if charging is to be done.
         // Otherwise the charge state will immediately proceed to the discharge state.

         if(charge_time) { 
            tft.fillScreen(ST77XX_BLACK);
            tft.setTextSize(3);
            tft.setCursor(50,70);
            tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
            tft.print("Charging");
            digitalWrite(CHARGE_RELAY_PIN,RELAY_ON_VAL);
         }
      }
  } else if(state == S_CHARGE) {
      if(secs != state_seconds) { 
         state_seconds = secs;
         if(secs >= charge_time) {

            // Charge time has elapsed, turn the relay off

            digitalWrite(CHARGE_RELAY_PIN,RELAY_OFF_VAL);

            if(do_discharge) {

               // Discharging is selected.  Recheck polarity to be absolutely sure we don't burn
               // out an ADC pin by turning on a battery relay

               check_polarity();

               // Turn on the discharge relays for all six channels
               // (up to this point, we don't actually know which channels have a battery connected)

               for(int i=0; i<6; i++) {
                  millivolt_seconds[i] = 0;
                  millijoules[i] = 0;
                  discharging[i] = 1;
                  digitalWrite(channels[i].relay_pin,RELAY_ON_VAL);
               }
               draw_grid();
               analog_ringbuf_ptr = 0;
               state = S_DISCHARGE;
               start_time = millis();
               state_seconds = -1;     // Make sure a state_seconds trigger happens right away
            } else {

               // No discharge selected.  Just freeze with "Done Charging" until powerdown

               tft.fillScreen(ST77XX_BLACK);
               tft.setTextSize(3);
               tft.setCursor(0,110);
               tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
               tft.print("Done Charging");
               for(;;) {} // freeze here
            }
         } else {

            // Charging is ongoing.  Display HH:MM:SS of time left

            tft.setTextSize(3);
            tft.setCursor(50,150);
            tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
            secs = charge_time - secs;
            char time_str[20];
            int s = secs % 60;
            int m = (secs/60) % 60;
            int h = secs/3600;
            sprintf(time_str,"%02d:%02d:%02d",h,m,s);
            tft.print(time_str);
         }
      }
   } else {
      // Discharge state

      // Arduino is good for about 10K analog reads per second.  We have a ring buffer of 50
      // and 6 inputs, so that's 300.  If we further read each input 20 times, that's 6000.
      // We read so many times to average out any noise on the inputs.  Also the reading is
      // done in interleaved fashion to space out the individual reads per channel.

      // This focus on eliminating noise, both in software and in hardware, gives sharp graph lines
      // and also more reliable milliamp hour readings.

      // Note that we do one ring buffer entry per pass through loop().  As this
      // theoretically takes around 120/10K = 12 milliseconds, this won't cause significant
      // jitter in the 1-second ticks and still, as long as 50*.012=0.6 seconds are availabe
      // between the 1 second processing runs the whole ring buffer will be updated.  Even if
      // it wasn't, e.g. only 3/4 of the ring buffer was updated, it would still be a meaningful
      // running average.

      // Zero out the current ring buffer entries
      for(int i=0;i<6;i++) analog_ringbuf[analog_ringbuf_ptr][i] = 0;

      // Then accumulate 20 ADC readings in each
      for(int j=0;j<20;j++) {
         for(int i=0;i<6;i++) {
            analog_ringbuf[analog_ringbuf_ptr][i] += analogRead(channels[i].adc_pin);
         }
      }

      // Go to the next ring buffer entry in circular fashion
      analog_ringbuf_ptr = (analog_ringbuf_ptr+1) % ANALOG_RINGBUF_SIZE;

      // Wait a couple of seconds at the start of the discharge state to let things stabilize.
      // Note that energy from the batteries in these two seconds isn't counted.  Oh well.

      if(secs != state_seconds && secs > 2) {
         state_seconds = secs;
 
         // Format a time string to use with all channels
         char time_str[50];
         int s = secs % 60;
         int m = (secs/60) % 60;
         int h = secs/3600;
         sprintf(time_str,"%02d:%02d:%02d     ",h,m,s);

         tft.setTextSize(1);
   
         int graph_x = secs/120;   // 30*120 = 3600 = 1 hour

         // Step through the channels that are still in "discharging" state.  Initially
         // that's all six of them.  Channels that have no battery installed will stop
         // due to undervoltage (less than 1.0 volt) before any data is printed.

         for(int i=0; i<6; i++) {
            // For every horizontal graph pixel, process the channels in a different
            // order.  This is because pixels are printed every second; the last channel
            // to be processed before an X increment determines the colour if several
            // graph lines are on top of each other.  Changing the processing order
            // causes each colour to get its turn if graph lines are on top of each other.
            int ch = (i+graph_x) % 6;

            if(discharging[ch]) {

               // Instead of keeping a running average, just add up the entire analog
               // ring buffer whenever needed

               unsigned long adc = 0;
               for(int j=0; j<ANALOG_RINGBUF_SIZE; j++) adc += analog_ringbuf[j][ch];

               // The following conversion is about right based on voltmeter readings.
               // Note that this is for ANALOG_RINGBUF_SIZE=50 and has to be scaled
               // if the ring buffer size is changed.

               int millivolts = (100L*adc+20165) / 40330;

               if(millivolts < 1000) {

                  // Done discharging this channel, or no battery present, in which
                  // case stop before printing any data.

                  digitalWrite(channels[ch].relay_pin,RELAY_OFF_VAL);  // Turn off the discharge relay
                  discharging[ch] = 0;
                  continue;
               } else {
                  millivolt_seconds[ch] += millivolts;

                  // This is actually 3.3x millijoules because we don't divide by R.
                  millijoules[ch] += ((long)millivolts*(long)millivolts+500L) / 1000;

                  // To convert from millivolt-seconds to milliamp-hours we need to divide by 3.3 
                  // to get the milliamps, and 3600 to get the hours.

                  int milliamp_hours = (millivolt_seconds[ch]+5940) / 11880;

                  // The first 12 characters of time_str were set earlier and are the same for
                  // all six channels.  The rest is different per channel

                  char volt_string[20];
                  if(button_state) {
                     // The millijoule accumulator still needs to be divided by 3.3 to get actual
                     // millijoules.  Further dividing by 3600 would give milliwatt hours, so dividing
                     // by 3.3*36000 gives centiwatt hours which we then display in n.nn format as Watt hours

                     // 3.3*36000 = 118800

                     int centiwatt_hours = (millijoules[ch]+59400L) / 118800L;

                     sprintf(volt_string,"%d.%02d Wh   ",centiwatt_hours/100,centiwatt_hours%100);
                  } else {
                     // If the button is held down, display voltages
                     int centivolts = (millivolts+5) / 10;
                     sprintf(volt_string,"%d.%02d Volt ",centivolts/100,centivolts%100);
                  }
                  tft.setCursor(20,10*ch);
                  tft.setTextColor(channels[ch].colour,ST77XX_BLACK);
                  sprintf(time_str+12,"%s %d mAh",volt_string,milliamp_hours);
                  tft.print(time_str);
   
                  // Plot a graph pixel
                  // Horizontal scale is 30 pixels per hour
                  // Vertical scale is 44 pixels per 100mV above 1.0V
                  //   = 0.44 pixel per millivolt

                  int graph_y = ((millivolts-1000) * 44 + 22) / 100;

                  // Pixels are drawn once per second, so any noise in the Y axis would
                  // show up as a thicker line.  Plotting once per second ensures no discontinuity
                  // in the graph if the voltage falls quickly i.e. more than one vertical pixel
                  // per 2 minute horizontal pixel

                  if(graph_x < 240 && graph_y >= 0 && graph_y < 180) {
                     tft.drawPixel(graph_x, 239-graph_y, channels[ch].colour);
                  }
               }
            }
         }
      }
   }
}

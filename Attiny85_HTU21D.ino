#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

#include <TinyWireM.h>
#include <Adafruit_HTU21DF.h>
//#include <SendOnlySoftwareSerial.h>

#define DOT 75           // Duration of a dot
#define DASH 375         // Duration of a dash
#define INNER_PAUSE 125  // Duration of pause between dot and dash in a character 
#define CHAR_BREAK 250   // Duration of pause between characters
#define START_FAST_REP 5 // Number of times message is repeated at startup with short delay

Adafruit_HTU21DF htu = Adafruit_HTU21DF();
//SendOnlySoftwareSerial mySerial (4);  // Tx pin

float temp, hum;
bool warn;

// *** BEGIN POWER SAVE CODE ** //

ISR(WDT_vect) {
  // Don't do anything here but we must include this
  // block of code otherwise the interrupt calls an
  // uninitialized interrupt handler.
}

void watchdogSetup(){
  //MCU MicroController Unit
  //MCUSR = MCU Status Register
  //WDTCR = WatchDog Timer Control and Register
  //WDTCSR = WatchDog Timer Control and Status Register
  //WDRF = WatchDog Reset Flag
  //WDCE = WatchDog Change Enable
  //WDE = WatchDog Enable
  //WDP3-0 = WatchDog Prescaler bits as follows
  
  //WDP3 - WDP2 - WPD1 - WDP0 - time
  // 0      0      0      0      16 ms
  // 0      0      0      1      32 ms
  // 0      0      1      0      64 ms
  // 0      0      1      1      0.125 s
  // 0      1      0      0      0.25 s
  // 0      1      0      1      0.5 s
  // 0      1      1      0      1.0 s
  // 0      1      1      1      2.0 s
  // 1      0      0      0      4.0 s
  // 1      0      0      1      8.0 s

  static byte fast = START_FAST_REP;

  // Reset the watchdog reset flag
  bitClear(MCUSR, WDRF);
  // Start timed sequence
  bitSet(WDTCR, WDCE); //Watchdog Change Enable to clear WD
  bitSet(WDTCR, WDE); //Enable WD

  if (fast){
    // Set new watchdog timeout value to 2 seconds
    bitSet(WDTCR, WDP2);
    bitSet(WDTCR, WDP1);
    bitSet(WDTCR, WDP0);
    fast--;     
  } else {
    // Set new watchdog timeout value to 8 seconds
    bitSet(WDTCR, WDP3);
    bitSet(WDTCR, WDP0);     
  }
  // Enable interrupts instead of reset
  bitSet(WDTCR, WDIE);
}

void goSleep(){
  watchdogSetup(); //enable watchDog
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_mode();
  //disable watchdog after sleep
  wdt_disable();
}

// *** END POWER SAVE CODE ** //

// *** BEGIN HUMIDITY / TEMPERATURE MEASUREMENT CODE ** //

void dot(){
  digitalWrite(LED_BUILTIN, HIGH);
  delay(DOT);
  digitalWrite(LED_BUILTIN, LOW);
  delay(INNER_PAUSE);
}

void dash(){
  digitalWrite(LED_BUILTIN, HIGH);
  delay(DASH);
  digitalWrite(LED_BUILTIN, LOW);
  delay(INNER_PAUSE);
}

void setup() {
  // *** BEGIN POWER SAVE CODE ** //
  //we don't do any analog to digital conversion here, nor using timer 1
  power_adc_disable();
  power_timer1_disable();
  //change all pin to OUTPUT LOW
  for (byte i = 0; i <= 5; i++){
    pinMode (i, OUTPUT);    
    digitalWrite (i, LOW);  
  }
  // *** END POWER SAVE CODE ** //
  
//  pinMode(LED_BUILTIN, OUTPUT);
//  mySerial.begin(9600);

  if (!htu.begin()) {
//    mySerial.println("Couldn't find sensor!");
    //SOS ... --- ...
    dot(); dot(); dot(); delay(CHAR_BREAK);
    dash(); dash(); dash(); delay(CHAR_BREAK);
    dot(); dot(); dot(); delay(CHAR_BREAK);
    while (1);
  }
}


void loop() {
  temp = htu.readTemperature();
  //compensated humidity calculation accodring to datasheet
  hum = htu.readHumidity() + (25 - temp) * -0.15;

  /*
  mySerial.print("Temp: ");
  mySerial.print(temp);
  mySerial.print("\tC.Hum: "); 
  mySerial.println(hum);
  delay(1000);
  */

  warn = false;

  if (hum > 75 || hum < 25 || temp < 5 || temp > 50){
    //[V]ERY ...-
    dot(); dot(); dot(); dash(); delay(CHAR_BREAK);
  }

  if (hum > 60){
    //[M]OIST --
    dash(); dash(); delay(CHAR_BREAK);
    warn = true;
  } else if (hum < 40){
    //[D]RY -..
    dash(); dot(); dot(); delay(CHAR_BREAK);
    warn = true;
  }
  
  if (temp < 15){
    //[C]OLD -.-.
    dash(); dot(); dash(); dot(); delay(CHAR_BREAK);
    warn = true;
  } else if (temp > 40){
    //[H]OT ....
    dot(); dot(); dot(); dot(); delay(CHAR_BREAK);
    warn = true;
  }

  if (! warn){
    //alive bip
    dot(); delay(CHAR_BREAK);
  }
  
  //delay(5000);
  //go sleep for N seconds
  goSleep();
}

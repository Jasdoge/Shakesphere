#include <arduino.h>
#include <avr/sleep.h>
// Pin definitions
#define PIN_R PB0
#define PIN_G PB1
#define PIN_B PB2
#define SENSOR PB3
#define RNG PB4


#define DURATION 500				// MS shake needed to activate. Lower = faster but less secure activation
#define SHAKE_ALLOWANCE 300			// MS between shake readings to consider it no longer being shaked
#define SLEEP 10800000				// MS before auto turn off (10800000 = 3h)
#define WAKEUP_GRACE_PERIOD 1000	// MS before entering sleep mode after turning the lamp off. This is to make it more responsive when turning it on and off rapidly to change colors. Higher = more responsive, Lower = Less power consumed from accidental bumps and shakes.

unsigned long SHAKE_STARTED = 0;				// MS when we started shaking.
unsigned long SHAKE_LAST_HIGH = 0;				// MS when the last LOW was detected on SENSOR.
bool ON = false;								// Whether the light is on or not
unsigned long TIME_LIT = 0;						// MS when the light was turned on (0 if off)
unsigned long TIME_WOKE = 0;					// MS when the attiny woke up from sleep


// After successfully shaking the sphere.
// Toggles the light
void onShake(){

	ON = !ON;	// Change on state

	byte color = 0;	// Color 0 = off
	if( ON ){
		// Since each light can only be on or off, we'll assign them a bit each
		byte c = millis()%6;
		color = 0b001;
		if( c == 0 )
			color = 0b010;
		if( c == 1 )
			color = 0b100;
		if( c == 2 )
			color = 0b110;
		if( c == 3 )
			color = 0b011;
		if( c == 4 )
			color = 0b101;
	}

	// Write the colors
	digitalWrite(PIN_R, !(color & 0b1));
	digitalWrite(PIN_G, !(color & 0b10));
	digitalWrite(PIN_B, !(color & 0b100));
}

ISR(PCINT0_vect){}

void sleep(){
	
	// Turns off
	ON = true;
	onShake();
	
	// Enters sleep mode
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // sleep mode is set here
    sleep_enable();                          // enables the sleep bit in the mcucr register so sleep is possible
	
	GIMSK = 0b00100000;    // turns on pin change interrupts
  	PCMSK = 0b00001000;    // turn on interrupt on pins PB3

    sleep_mode();                          // here the device is actually put to sleep!!
	// Waking up
    sleep_disable();                       // first thing after waking from sleep: disable sleep...
    detachInterrupt(0);                    // disables interrupton pin 3 so the wakeUpNow code will not be executed during normal running time.	
	TIME_WOKE = millis();

}

// IT BEGINS
void setup(){
	
	// Set all LEDs to OUTPUT and HIGH (because we're using common anode RGB, otherwise you'd use HIGH to turn on)
	pinMode(PIN_R, OUTPUT);
	pinMode(PIN_G, OUTPUT);
	pinMode(PIN_B, OUTPUT);
	
	// Enable the sensor
	pinMode(SENSOR, INPUT_PULLUP);
	//digitalWrite(SENSOR, HIGH);

	// Power saving measures
	ADCSRA &= ~ bit(ADEN); // disable the ADC

	// Set woke time to prevent it from sleeping immediately
	TIME_WOKE = millis();

}


void loop(){

	const unsigned long ms = millis();		// Getting the time

	// Auto turnoff after SLEEP milliseconds
	if( ON && ms-TIME_LIT > SLEEP ){
		sleep();
	}

	// If SENSOR is LOW, we're shaking it or holding it upside down
	else if( !digitalRead(SENSOR) ){

		SHAKE_LAST_HIGH = ms;	// Sets so we can tell that we're still shaking it or holding it upside down

		// We haven't detected shaking yet, so set this as the start time for shake
		if( !SHAKE_STARTED ){
			// Debug: Blink once when shake starts
			SHAKE_STARTED = ms;
		}
		// We have been shaking for enough time, toggle the lamp
		else if( ms-SHAKE_STARTED > DURATION ){			

			SHAKE_STARTED = 0;	// Indicate we're no longer shaking
			
			// Toggle the lights
			onShake();
			
			// If the light is now off, schedule the sphere to sleep after a timeout unless we start shaking it again
			if( !ON )
				TIME_WOKE = ms;
			// Otherwise set the time we lit it so we know when to auto turn off to save power
			else
				TIME_LIT = ms;
			
		}

	}

	// There has been no shake detected for SHAKE_ALLOWANCE milliseconds since the last detected shake
	else if( ms-SHAKE_LAST_HIGH > SHAKE_ALLOWANCE ){
		
		// Clear the shake timers
		SHAKE_STARTED = 0;
		SHAKE_LAST_HIGH = 0;

		// If we're not shaking and the device isn't on. Sleep unless we're in the grace period
		if( !ON && ms-TIME_WOKE > WAKEUP_GRACE_PERIOD )
			sleep();

	}


}
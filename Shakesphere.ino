#include <avr/sleep.h>

//#define DEBUG_BLINKS

//#define LEGACY
#ifndef LEGACY
	#define PIN_R PB0
	#define PIN_G PB1
	#define PIN_B PB2
	#define SENSOR PCINT3
	#define RNG PB4
#else
	#define PIN_R PB0
	#define PIN_G PB1
	#define PIN_B PB3
	#define SENSOR PCINT2
	#define RNG PB4
#endif

#define SHAKE_ON LOW


#define DURATION 500				// MS shake needed to activate. Lower = faster but less secure activation
#define SHAKE_ALLOWANCE 300			// MS between shake readings to consider it no longer being shaked
#define SLEEP 10800000				// MS before auto turn off (10800000 = 3h)

unsigned long SHAKE_STARTED = 0;				// MS when we started shaking.
unsigned long SHAKE_LAST_HIGH = 0;				// MS when the last LOW was detected on SENSOR.
bool ON = false;								// Whether the light is on or not
unsigned long TIME_LIT = 0;						// MS when the light was turned on (0 if off)


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

	#ifdef DEBUG_BLINKS
		for( byte i=0; i<3; ++i ){
			digitalWrite(PIN_R, LOW);
			delay(100);
			digitalWrite(PIN_R, HIGH);
			delay(100);
		}
	#endif

	// Turns off
	ON = true;
	onShake();
	
	GIMSK |= _BV(PCIE);    // turns on pin change interrupts
  	PCMSK |= _BV(SENSOR);    // turn on interrupt on sensor pin

	// Enters sleep mode
    //sleep_enable();                          // enables the sleep bit in the mcucr register so sleep is possible
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);    // replaces above statement
	sleep_mode();

  	PCMSK &= ~_BV(SENSOR);    // interrupt off
	#ifdef DEBUG_BLINKS
		digitalWrite(PIN_R, LOW);
		delay(1000);
		digitalWrite(PIN_R, HIGH);
	#endif
	SHAKE_LAST_HIGH = millis();

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
	ADCSRA &= ~_BV(ADEN); // disable the ADC
}


void loop(){

	const unsigned long ms = millis();		// Getting the time

	// Auto turnoff after SLEEP milliseconds
	if( ON && ms-TIME_LIT > SLEEP ){
		sleep();
		return;
	}
	
	
	// If SENSOR is LOW, we're shaking it or holding it upside down
	if( digitalRead(SENSOR) == SHAKE_ON ){

		SHAKE_LAST_HIGH = ms;	// Sets so we can tell that we're still shaking it or holding it upside down

		// We haven't detected shaking yet, so set this as the start time for shake
		if( !SHAKE_STARTED )
			SHAKE_STARTED = ms;

		// We have been shaking for enough time, toggle the lamp
		else if( ms-SHAKE_STARTED > DURATION ){			

			SHAKE_STARTED = 0;	// Indicate we're no longer shaking
			
			// Toggle the lights
			onShake();
			
			// set the time we lit it so we know when to auto turn off to save power
			if( ON )
				TIME_LIT = ms;
			
		}
		return;
	}

	// There has been no shake detected for SHAKE_ALLOWANCE milliseconds since the last detected shake
	if( ms-SHAKE_LAST_HIGH > SHAKE_ALLOWANCE ){
		
		// Clear the shake timers
		SHAKE_STARTED = 0;
		SHAKE_LAST_HIGH = 0;

		// If we're not shaking and the device isn't on. Sleep unless we're in the grace period
		if( !ON )
			sleep();

	}

}
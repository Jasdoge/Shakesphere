#define ATTINY13
#include <avr/sleep.h>
//#include <avr/power.h>    // Power management

// Pin definitions
#define PIN_R 0
#define PIN_G 1
#define PIN_B 3
#define SENSOR 2

#define DURATION 500				// MS shake needed to activate. Lower = faster but less secure activation
#define SHAKE_ALLOWANCE 300			// MS between shake readings to consider it no longer being shaked
#define SLEEP 10800000				// MS before auto turn off (10800000 = 3h)
#define WAKEUP_GRACE_PERIOD 1000	// MS before entering sleep mode after turning the lamp off. This is to make it more responsive when turning it on and off rapidly to change colors. Higher = more responsive, Lower = Less power consumed from accidental bumps and shakes.

unsigned long SHAKE_STARTED = 0;					// MS when we started shaking.
unsigned long SHAKE_LAST_HIGH = 0;				// MS when the last LOW was detected on SENSOR.
bool ON = false;						// Whether the light is on or not
unsigned long TIME_LIT = 0;						// MS when the light was turned on (0 if off)
unsigned long TIME_WOKE = 0;						// MS when the attiny woke up from sleep


// After successfully shaking the sphere.
// Toggles the light
void onShake(){

	ON = !ON;	// Change on state

	byte color = 0;	// Color 0 = off
	if( ON ){
		// Since each light can only be on or off, we'll assign them a bit each
		const byte R = 0x1, G = 0x2, B = 0x4;
		byte c = random(0,6);
		switch(c){
			case 0:
				color = R|G;
			case 1:
				color = R|B;
			case 2:
				color = G|B;
			case 3:
				color = R;
			case 4:
				color = G;
			case 5:
				color = B;
			
		}
	}

	// Write the colors
	digitalWrite(PIN_R, !(color&1));
	digitalWrite(PIN_R, !(color&2));
	digitalWrite(PIN_R, !(color&4));
}

// Blinks a pin N times. Used for init and debugging
#ifndef ATTINY13
	void blinkPin( byte pin, byte times ){

		for( byte i =0; i<times; ++i ){
			digitalWrite(pin, LOW);
			delay(100);
			digitalWrite(pin, HIGH);
			delay(100);
		}

	}
#endif

// Not sure if this is needed, and never actually triggers. But attachInterrupt uses it
void onWakeup(){}

void sleep(){
	
	// Turns off
	ON = true;
	onShake();
	
	// Enters sleep mode
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // sleep mode is set here
    sleep_enable();                          // enables the sleep bit in the mcucr register so sleep is possible
    attachInterrupt(0, onWakeup, LOW);     // use interrupt 0 (pin 2) and run function wakeUpNow when pin 2 gets LOW
    sleep_mode();                          // here the device is actually put to sleep!!
	// Waking up
    sleep_disable();                       // first thing after waking from sleep: disable sleep...
    detachInterrupt(0);                    // disables interrupton pin 3 so the wakeUpNow code will not be executed during normal running time.	
	TIME_WOKE = millis();
	//blinkPin(PIN_G, 4);

}

// IT BEGINS
void setup(){

	// Set all LEDs to OUTPUT and HIGH (because we're using common anode RGB, otherwise you'd use HIGH to turn on)
	pinMode(PIN_R, OUTPUT);
	digitalWrite(PIN_R, HIGH);
	pinMode(PIN_G, OUTPUT);
	digitalWrite(PIN_G, HIGH);
	pinMode(PIN_B, OUTPUT);
	digitalWrite(PIN_B, HIGH);
	
	// Blink the green pin 4 times to signify that we now have power (useful for debugging)
	#ifndef ATTINY13
		blinkPin(PIN_G, 4);
	#endif

	// Enable the sensor
	pinMode(SENSOR, INPUT_PULLUP);
	//digitalWrite(SENSOR, HIGH);

	// Power saving measures
	ADCSRA &= ~ bit(ADEN); // disable the ADC
	// 13 doesn't have these
	#ifndef ATTINY13
		//power_adc_disable();
		power_usi_disable();
		power_timer1_disable();
	#endif


	// Set woke time to prevent it from sleeping immediately
	TIME_WOKE = millis();

}


void loop(){

	const unsigned long ms = millis();		// Getting the time

	// Auto turnoff after SLEEP milliseconds
	if( ON && ms-TIME_LIT < SLEEP ){
		sleep();
		return;
	}

	// If SENSOR is LOW, we're shaking it or holding it upside down
	if( !digitalRead(SENSOR) ){

		SHAKE_LAST_HIGH = ms;	// Sets so we can tell that we're still shaking it or holding it upside down

		// We haven't detected shaking yet, so set this as the start time for shake
		if( !SHAKE_STARTED )
			SHAKE_STARTED = ms;
		// We have been shaking for enough time, toggle the lamp
		else if( ms-SHAKE_STARTED < DURATION ){			

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
	else if( ms-SHAKE_LAST_HIGH < SHAKE_ALLOWANCE ){
		
		// Clear the shake timers
		SHAKE_STARTED = 0;
		SHAKE_LAST_HIGH = 0;

		// If we're not shaking and the device isn't on. Sleep unless we're in the grace period
		if( !ON && ms > TIME_WOKE+WAKEUP_GRACE_PERIOD )
			sleep();

	}


}
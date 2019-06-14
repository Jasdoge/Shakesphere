#include <avr/sleep.h>

unsigned long last_ms = 0;

void doSleep(){  

	digitalWrite(PB0, LOW);
	delay(50);
	digitalWrite(PB0, HIGH);
	GIMSK |= _BV(PCIE);                     // Enable Pin Change Interrupts
    PCMSK |= _BV(PCINT3);                   // Use PB3 as interrupt pin
    ADCSRA &= ~_BV(ADEN);                   // ADC off
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);    // replaces above statement
	sleep_mode();
    PCMSK &= ~_BV(PCINT3);                   // Use PB3 as interrupt pin

	last_ms = 0;
	//long ms = millis();

} 
ISR(PCINT0_vect) { }

void setup(){

	pinMode(PB0, OUTPUT);
	pinMode(PB1, OUTPUT);
	pinMode(PB2, OUTPUT);
	pinMode(PCINT3, INPUT_PULLUP);
	pinMode(PB4, OUTPUT);
	// Flash twice on boot

	digitalWrite(PB1, HIGH);
	digitalWrite(PB2, HIGH);
	
	digitalWrite(PB0, LOW);
	delay(100);
	digitalWrite(PB0, HIGH);
	delay(100);
	digitalWrite(PB0, LOW);
	delay(100);
	digitalWrite(PB0, HIGH);
	delay(100);
}//main

void loop(){

	const long ms = millis();
	if( !last_ms ){
		digitalWrite(PB0, LOW);
		delay(2000);
		digitalWrite(PB0, HIGH);
		last_ms = ms;
	}

	if( ms == last_ms ){
		digitalWrite(PB0, LOW);
		delay(100);
		digitalWrite(PB0, HIGH);
		delay(100);
	}

	if( ms-last_ms > 2000 ){
		// one long flash after waking up
		doSleep();   // go to sleep and wait for interrupt...
	}
}
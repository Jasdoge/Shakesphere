#include <avr/sleep.h>

void doSleep(){  

	GIMSK |= (1<<PCIE);                     // Enable Pin Change Interrupts
    PCMSK |= (1<<PCINT3);                   // Use PB3 as interrupt pin
    ADCSRA &= ~(1<<ADEN);                   // ADC off

	WDTCR |= (1<<WDP3 )|(0<<WDP2 )|(0<<WDP1)|(1<<WDP0); // 8s
	WDTCR |= (1<<WDTIE);
	sei();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);    // replaces above statement
	sleep_mode();

} 
ISR(WDT_vect) { }

void setup(){

	pinMode(PB0, OUTPUT);
	pinMode(PB1, OUTPUT);
	pinMode(PB2, OUTPUT);
	pinMode(PB3, OUTPUT);
	pinMode(PB4, OUTPUT);
	// Flash twice on boot
	digitalWrite(PB4, HIGH);
	delay(100);
	digitalWrite(PB4, LOW);
	delay(100);
	digitalWrite(PB4, HIGH);
	delay(100);
	digitalWrite(PB4, LOW);
	delay(100);
}//main

void loop(){
	// one long flash after waking up
	digitalWrite(PB4, HIGH);
	delay(2000);
	digitalWrite(PB4, LOW);
	doSleep();   // go to sleep and wait for interrupt...
}
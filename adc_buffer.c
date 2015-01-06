#include <util/delay.h>

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "adc_sampler.h"
#include "print.h"

#define BUFSIZE 4
static volatile uint8_t buffer[BUFSIZE], head, tail, count;

void adc_start(uint8_t mux, uint8_t aref) {
	ADCSRA = (1<<ADEN) | ADC_PRESCALER;     // enable the ADC, interrupt disabled
	ADCSRB = (1<<ADHSM) | (mux & 0x20) | ADC_TRIGGER_EXT_INT0;
	ADMUX = aref | (mux & 0x1F);            // configure mux and ref
	
	head = 0;
	tail = 0;
	buffer[head] = 0x00;                               // clear the buffer
	
	ADCSRA = (1<<ADEN) | (1<<ADATE) | (1<<ADIE) | ADC_PRESCALER;
	EICRA |= ((1<<ISC01) | (1<<ISC00));     // config ext INT0 rising edge
	sei();
}

uint8_t adc_available(void) {
	uint8_t c = count;
	
	return c;
}


uint8_t adc_read(void) {
    uint8_t val, h, t;
    do {
        h = head;
        t = tail;                   // wait for data in buffer
    } while (h == t);
	
	
    if (++t >= BUFSIZE) {
		t = 0;
	}
	
    val = buffer[t];                // remove 1 sample from buffer
    
    tail = t;
	count--;
	
    return val;
}

ISR(ADC_vect) {
	// Mask off the higher voltage value to ignore jitter and clamp to 4 bits.
	uint8_t btn = (ADC >> 4) & 0x0F;
	
	EIFR = 0x01;
	
	//buffer = btn;
	uint8_t h = head + 1;
	if (h >= BUFSIZE) {
		h = 0;
	}
	
	if (h != tail) {                // if the buffer isn't full
        buffer[h] = btn;            // put new data into buffer
        head = h;
		
		count++;
    }
	
	
	print("ADC INT0: ");
	phex(btn);
	print("\t");
	phex(h);
	print("\t");
	phex(tail);
	
	print("\t");
	phex(adc_available());
	print("\n");
	usb_debug_flush_output();
}

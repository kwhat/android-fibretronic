// teensy_loader_cli -mmcu=atmega32u4 ./fibretronic.hex 
// teensy_hid_listen
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include "adc_sampler.h"
#include "usb_rawhid_debug.h"
#include "print.h"

// Teensy 2.0: LED is active high
#if defined(__AVR_ATmega32U4__) || defined(__AVR_AT90USB1286__)
#define LED_ON		(PORTD |= (1<<6))
#define LED_OFF		(PORTD &= ~(1<<6))

// Teensy 1.0: LED is active low
#else
#define LED_ON	(PORTD &= ~(1<<6))
#define LED_OFF	(PORTD |= (1<<6))
#endif

#define LED_CONFIG	(DDRD |= (1<<6))

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))
#define CPU_16MHz       0x00
#define CPU_8MHz        0x01
#define CPU_4MHz        0x02
#define CPU_2MHz        0x03
#define CPU_1MHz        0x04
#define CPU_500kHz      0x05
#define CPU_250kHz      0x06
#define CPU_125kHz      0x07
#define CPU_62kHz       0x08

#define HEX(n) (((n) < 10) ? ((n) + '0') : ((n) + 'A' - 10))

#define HIGH 0x1
#define LOW  0x0

#define CHANGE 1
#define FALLING 2
#define RISING 3

#define BTN_NONE			0x00
#define BTN_FWD 			1 << 0
#define BTN_BACK			1 << 1
#define BTN_PLAY			1 << 2
#define BTN_PLUS			1 << 3
#define BTN_MIN				1 << 4



//static const uint8_t adc_val_to_hid_PGM[] = {
static const uint8_t adc_val_to_hid_PGM[] = {
	BTN_NONE,	/* 0 */
	BTN_NONE,	/* 1 */
	BTN_BACK,	/* 2 */
	BTN_PLUS,	/* 3 */
	BTN_NONE,	/* 4 */
	BTN_NONE,	/* 5 */
	BTN_NONE,	/* 6 */
	BTN_NONE,	/* 7 */
	BTN_NONE,	/* 8 */
	BTN_PLAY,	/* 9 */
	BTN_NONE,	/* A */
	BTN_MIN,	/* B */
	BTN_NONE,	/* C */
	BTN_FWD,	/* D */
	BTN_NONE,	/* E */
	BTN_NONE	/* F */
};

static volatile int16_t val = BTN_NONE;
uint8_t buffer[1];

uint8_t count;
uint8_t down;

int main(void) {
	//CPU_PRESCALE(CPU_125kHz);
	//_delay_ms(1);
	//CPU_PRESCALE(CPU_16MHz);
	
	//CPU_PRESCALE(CPU_500kHz);
	CPU_PRESCALE(CPU_62kHz);
	
	LED_CONFIG;
	
	power_adc_disable();
	power_usart0_disable();
	power_spi_disable();
	power_twi_disable();
	power_timer0_disable();
	power_timer1_disable();
	power_timer2_disable();
	power_timer3_disable();
	power_usart1_disable();
	power_usb_disable();
	
	LED_ON;
	_delay_ms(2000);
	LED_OFF;
	while(1) {
		// Enable the sleep bit in the MCUCR register so sleep is possible.
		sleep_enable();
		
		// attachInterrupt
		EIMSK |= (1 << INT0);
		
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		
		// Go to sleep!
		sleep_cpu();
		
		// Wake up here.
		sleep_disable();
	}
	
	// initialize the USB, but don't want for the host to
	// configure.  The first several messages sent will be
	// lost because the PC hasn't configured the USB yet,
	// but we care more about blinking than debug messages!
	usb_init();
	while (!usb_configured()); /* wait */
	
	count = 0;
	
	// Start ADC INT0 listener to read F0.
	adc_start(ADC_MUX_PIN_F0, ADC_REF_POWER);
	
	/*
	USBCON |= (1 << FRZCLK);             // Freeze the USB Clock              
	PLLCSR &= ~(1 << PLLE);              // Disable the USB Clock (PPL) 
	USBCON &=  ~(1 << USBE  );           // Disable the USB  
	blip(1,50);
	delay(8000);  // 5.3mA
	*/
	
	// Set INT0 to listen for hi-signal on pin 5 (D0).
	EICRA = (EICRA & ~((1<<ISC00) | (1<<ISC01))) | (RISING << ISC00);

	// Set PB0 - BP7 to OUTPUT mode.
	DDRB |= 0xFF;
	
	// Set PC6 & PC7 to OUTPUT mode.
	DDRC |= 0xC0;
	
	// Set PD1 - PD7 excluding PD0 to OUTPUT mode.
	DDRD |= 0xFE; // All: 0xFF
	
	// Set PE6 & PD2 to OUTPUT mode.
	DDRE |= 0x44;
	
	// Set PF1 & PF4 - PF7 excluding PF0 to OUTPUT mode.
	DDRF |= 0xF2; // All: 0xF3
	
	while (1) {
//		LED_ON;
		
		uint8_t size = adc_available();
		if (size) {
			LED_ON;
			
			cli();
			CPU_PRESCALE(CPU_500kHz);
			_delay_ms(1);
			sei();
			
			do {
				// Refresh the pin.
				int8_t hid = adc_val_to_hid_PGM[adc_read()];
				
				print("HID Key: ");
				phex(hid);
				print("\n");
				usb_debug_flush_output();
				
				buffer[0] = hid;
				usb_rawhid_send(buffer, 0);
				//usb_rawhid_recv(buffer, 0);
				
				//_delay_ms(50);
				
				buffer[0] = 0x00;
				usb_rawhid_send(buffer, 0);
			} while (--size);
			
			_delay_ms(200);
			
			cli();
			CPU_PRESCALE(CPU_62kHz);
			sei();
			
			LED_OFF;
		}
		
/*
		LED_OFF;
		// Enable the sleep bit in the MCUCR register so sleep is possible.
		sleep_enable();
		
		// attachInterrupt
		EIMSK |= (1 << INT0);
		
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		
		// Go to sleep!
		sleep_cpu();
		
		// Wake up here.
		sleep_disable();
//*/
	}
	
	// Disables all interrupts by clearing the global interrupt mask.
	cli();
	
	
}

ISR(INT0_vect) {
	//print("***INT0!\n");
	//usb_debug_flush_output();
	
	// Wake up!
	//sleep_disable();
	
	// detachInterrupt
	//EIMSK &= ~(1<<INT0);
}


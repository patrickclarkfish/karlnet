// Karl Palsson, 2010
// 
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h> 
#include <util/delay.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include "suart.h"
#include "karlnet.h"
#include "xbee-api.h"


#define XBEE_OFF          (PORTB |= (1<<PB4))
#define XBEE_ON         (PORTB &= ~(1<<PB4))
#define XBEE_CONFIG      (DDRB |= (1<<PB4))

#define SENSOR_DELAY_8_4sec 0
#define SENSOR_DELAY_5sec 104
#define SENSOR_DELAY_2_5sec 180
#define SENSOR_DELAY_1_8sec 200

#define VREF_VCC  0
#define VREF_11  (1<<REFS1)
#define VREF_256 (1<<REFS1) | (1<<REFS2)
#define MUXBITS_TEMP (0x03)
#define MUXBITS_CHANNEL2 (0x01)

#define ADC_ENABLE  (ADCSRA |= (1<<ADEN))
#define ADC_DISABLE  (ADCSRA &= ~(1<<ADEN))


void init_adc_regular(uint8_t muxBits) {
    ADMUX = VREF_256 | muxBits;
}

unsigned int adc_read(void)
{
    ADCSRA |= (1<<ADSC);               // begin the conversion
    while (ADCSRA & (1<<ADSC)) ;   // wait for the conversion to complete
    unsigned char lsb = ADCL;                              // read the LSB first
    return (ADCH << 8) | lsb;          // read the MSB and return 10 bit result
}

void init_adc_int_temp(void)
{
    ADMUX = VREF_11 | (1<<MUX3) | (1<<MUX2) | (1<<MUX1) | (1<<MUX0); // internal temp sensor
}


void init(void) {
	XBEE_CONFIG;
	XBEE_OFF;
	clock_prescale_set(0);
	TX_CONFIG;
        ADCSRA = (1<<ADPS2) | (1<<ADPS1); // prescale down to 125khz for accuracy

	// things we never need...
	power_timer0_disable();
	power_timer1_disable();
	power_usi_disable();

        // switch off the analog comparator
        ACSR &= ~(1<<ACIE);
        ACSR |= (1<<ACD);

        // Make sure no analog input pins have digital buffers
        DIDR0 = 0;  // we don't use _any_ digitial inputs
	
        // We're going to use the watchdog timer to wake us up from deep sleep...
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // Deeeeep sleeeep
    
        MCUSR &= ~(1<<WDRF);
        // start timed sequence
        WDTCR |= (1<<WDCE) | (1<<WDE);
        // set new watchdog timeout value
        WDTCR = (1<<WDP3);  // 4 secs
        WDTCR |= (1<<WDIE);

}

EMPTY_INTERRUPT(WDT_vect);


int main(void) {
	init();

        kpacket packet;
        packet.header = 'x';
        packet.version = 1;
        packet.nsensors = 3;

	while (1) {

		power_adc_enable();
                _delay_us(70);  // max internal vref startup time from datasheet
                ADC_ENABLE;
                init_adc_regular(MUXBITS_TEMP);
		unsigned int tmp36 = adc_read();
                init_adc_regular(MUXBITS_CHANNEL2);
                unsigned int tmp36_channel2 = adc_read();
		init_adc_int_temp();
                unsigned int internalTempSensor = adc_read();
                ADC_DISABLE;
		power_adc_disable();

                ksensor s1 = {36, tmp36};
                ksensor s2 = {'i', internalTempSensor};
                ksensor s3 = {36, tmp36_channel2};
                packet.ksensors[0] = s1;
                packet.ksensors[1] = s2;
                packet.ksensors[2] = s3;

		XBEE_ON;
		_delay_ms(15);  // xbee manual says 2ms for sleep mode 2, 13 for sleep mode 1
		xbee_send_16(1, packet);  // manually set my base station to address MY = 1
		XBEE_OFF;

		// now sleep!
		sei();
		sleep_mode();
		sleep_disable();
		cli();
	}
}


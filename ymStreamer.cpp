#include "ymStreamer.h"
#include "ym2149.h"
#include <Arduino.h>

namespace ym
{

#define TX_START()    UCSR0B |= _BV(TXEN0)  // Enable TX
#define TX_STOP()   UCSR0B &= ~_BV(TXEN0) // Disable TX
#define RX_START()    UCSR0B |= _BV(RXEN0)  // Enable RX
#define RX_STOP()   UCSR0B &= ~_BV(RXEN0) // Disable RX

#ifndef FOSC
#define  FOSC      16000000UL
#endif

#define _BAUD      57600         // Baud rate (9600 is default)
#define _DATA     0x03          // Number of data bits in frame = byte tranmission
#define _UBRR     (FOSC/16)/_BAUD - 1   // Used for UBRRL and UBRRH


void initialize_streamer(unsigned baudrate)
{
	// Not necessary; initialize anyway
	DDRD |= _BV(PD1);
	DDRD &= ~_BV(PD0);

	const uint32_t ubrr = (FOSC/16) / baudrate - 1;

	// Set baud rate; lower byte and top nibble
	UBRR0H = ((_UBRR) & 0xF00);
	UBRR0L = (uint8_t)((_UBRR) & 0xFF);

	TX_START();
	RX_START();

	// Set frame format = 8-N-1
	UCSR0C = (_DATA << UCSZ00);
}

void update_streamer()
{
	char regs[16];
	int reg = 0;

	for(;reg < 16;) {
	  // spin until data
	  while(!(UCSR0A & _BV(RXC0)));
	  regs[reg++] = (uint8_t)UDR0;
	}

	for(int i = 0; i < 14; ++i) {
	  send_data(i, regs[i]);
	}

}


}
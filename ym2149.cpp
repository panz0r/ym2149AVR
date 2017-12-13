#include "ym2149.h"
#include <Arduino.h>


namespace ym
{

// Pins
// ym2149			m328p		Uno
// clock			PB3			11
// BDIR				PB4			12
// BC1				PB5			13
// Reset						10

// DA0				PC0			A0		0x1
// DA1				PC1			A1		0x2
// DA2				PD2			2		0x4
// DA3				PD3			3		0x8
// DA4				PD4			4		0x10
// DA5				PD5			5		0x20
// DA6				PD6			6		0x40
// DA7				PD7			7		0x80

#define BC1 (1<<PORTC3)
#define BDIR (1<<PORTC2)

#define ADDRESS_MODE (BC1 | BDIR)
#define WRITE_MODE (BDIR)
#define READ_MODE (BC1)

#define BUS_CTL_MASK (0xc)
#define BUS_CTL_PORT PORTC

#define LOWDATA_MASK (0x3)
#define HIDATA_MASK (0xfc)
#define LOWDATA_PORT PORTC
#define HIDATA_PORT PORTD

void set_data_output()
{
	DDRC |= 0x3; // low 2 data bits
	DDRD |= 0xfc; // high 6 data bits
}

void set_data_input()
{
	DDRC &= ~0x3;
	DDRD &= ~0xfc;
}

void init_clock()
{
	// Open OS2A for output
	DDRB |= 0x01 << PORTB3;

	// clear all bits on the TCCR2 registers
	TCCR2A = 0;
	TCCR2B = 0;
	TCNT2 = 0;

	// Toggle OC2A on compare match
	TCCR2A |= 0x01 << WGM21;
	TCCR2A |= 0x01 << COM2A0;

	// Use CLK I/O without prescaling
	TCCR2B |= 0x01 << CS20;

	// Set freq 4Mhz
	// (16 * 10 ^ 6) / (4 * 10 ^ 6) - 1 = 0x3
	OCR2A = 0x3;

}

void initialize()
{
	init_clock();

	// Set ctrl bus pins as output
	DDRC |= 0xc; // PC2 | PC3

	set_data_output();

	// reset
	pinMode(10, OUTPUT);
	digitalWrite(10, LOW);
	_delay_us(1.0);
	digitalWrite(10, HIGH);
	_delay_us(2000);

	// clear all registers (is this really needed? I think a reset clears all registers)
	for (int i = 0; i < 16; ++i) {
		send_data(i, 0);
	}
}


inline void set_address_mode()
{
	BUS_CTL_PORT = (BUS_CTL_PORT & ~BUS_CTL_MASK) | ADDRESS_MODE;
}

inline void set_write_mode()
{
	BUS_CTL_PORT = (BUS_CTL_PORT & ~BUS_CTL_MASK) | WRITE_MODE;
}

void set_read_mode()
{
	BUS_CTL_PORT = (BUS_CTL_PORT & ~BUS_CTL_MASK) | READ_MODE;
}

inline void set_inactive_mode()
{
	BUS_CTL_PORT = (BUS_CTL_PORT & ~BUS_CTL_MASK);
}

void set_address(int address)
{
	LOWDATA_PORT = (LOWDATA_PORT & ~LOWDATA_MASK) | (address & LOWDATA_MASK);
	HIDATA_PORT = (HIDATA_PORT & ~HIDATA_MASK) | (address & HIDATA_MASK);

	set_address_mode();
	_delay_us(1.0);
	set_inactive_mode();
	_delay_us(1.0);
}

void set_data(int data)
{
	LOWDATA_PORT = (LOWDATA_PORT & ~LOWDATA_MASK) | (data & LOWDATA_MASK);
	HIDATA_PORT = (HIDATA_PORT & ~HIDATA_MASK) | (data & HIDATA_MASK);

	set_write_mode();
	_delay_us(1.0);
	set_inactive_mode();
	_delay_us(1.0);
}

int read_data()
{
	set_data_input();

	set_read_mode();
	_delay_us(1.0);

	int data = 0;

	data = (PINC & LOWDATA_MASK);
	data |= (PIND & HIDATA_MASK);

	_delay_us(1.0);
	set_inactive_mode();
	_delay_us(1.0);

	set_data_output();

	return data;
}


void send_data(int address, int data)
{
	set_address(address);
	set_data(data);
}

int get_data(int address)
{
	set_address(address);
	return read_data();
}


}
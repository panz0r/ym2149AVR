#include "ymPlay.h"
#include "ymInstrument.h"
#include "ymFreq.h"

#include <Arduino.h>

namespace ym
{

/*
// YM 2149 registers

Register	Desc			B7	B6	B5	B4	B3	B2	B1	B0			Legend
R0			Chan A Freq		x	x	x	x	x	x	x	x			fine freq part
R1			Chan A Freq						x	x	x	x			coarse freq part
R2			Chan B Freq		x	x	x	x	x	x	x	x
R3			Chan B Freq						x	x	x	x
R4			Chan C Freq		x	x	x	x	x	x	x	x
R5			Chan C Freq						x	x	x	x
R6			Noise Freq					x	x	x	x	x			5 bit freq
									  Noise		 Tone
R7			Mixer					NC	NB	NA	TC	TB	TA			bit clear equals output
R8			Level A						M	x	x	x	x			B5 set means channel is controlled by evnvelope
R9			Level B						M	x	x	x	x
RA			Level C						M	x	x	x	x
RB			Envelope Freq	x	x	x	x	x	x	x	x			16 bit freq
RC			Envelope Freq	x	x	x	x	x	x	x	x
RD			Envelope Shape					x	x	x	x			CONT	ATT		ALT		HOLD

Tone freq equation: f = 2000000 / (16 * frequency)

Noise freq equation: f = 2000000 / (16 * frequency)

Envelope freq equation: f = 2000000 / (256 * frequency)



*/


static const uint32_t prescale_divider[]
{
	1,		(1 << CS10),
	8,		(1 << CS11),
	64,		(1 << CS11) | (1 << CS10),
	256,	(1 << CS12),
	1024,	(1 << CS12) | (1 << CS10),
};

extern uint32_t _sample_freq = 50;


struct Channel
{
	char note;
	char volume;
	int freq;
	bool on;
	
	bool has_instrument;
	Instrument instrument;
};

struct ChipState
{
	Channel channels[3];
	char mixer_state;
};

ChipState state = {};
void init_state()
{
	for (int i = 0; i < 3; ++i) {
		Channel &chan = state.channels[i];
		chan.note = 0;
		chan.volume = 0;
		chan.freq = 0;
		chan.on = false;
		chan.has_instrument = false;
	}

	state.mixer_state = 0xff;
}

// Interrupt callback
ISR(TIMER1_COMPA_vect) {
	
	PORTB |= (1<<PB1);

	for (int i = 0; i < 3; ++i) {
		Channel &channel = state.channels[i];
		int freq = channel.freq;
		int vol = channel.volume;
		if (channel.has_instrument) {
			apply_instrument(channel.note, channel.instrument, channel.freq, channel.volume);
		}

		// Set tone freq and volume
		//if (freq != channel.freq) {
			send_data(i * 2, freq & 0xff);
			send_data(i * 2 + 1, freq >> 8);
		//}
		//if (vol != channel.volume) {
			send_data(i + 8, channel.volume & 0xf);
		//}
	}

	// set mixer state
	//send_data(7, state.mixer_state | 0xf8);

	PORTB &= ~(1<<PB1);
}

// Timer1 interrupt


void init_timer(uint32_t sample_freq)
{
	_sample_freq = sample_freq;

	// Calculate best timer based on sample frequncy
	int prescaler = 4;
	uint32_t result = 0;
	for (; prescaler >= 0; --prescaler) {
		result = 16000000U / (prescale_divider[prescaler * 2] * sample_freq) - 1;

		if (result < 65536 && result > 100)
			break;
	}

#if defined(_DEBUG)
	Serial.println("best result:");
	Serial.println(prescaler);
	Serial.println(prescale_divider[prescaler * 2]);
	Serial.println(result);
	delay(1000);
#endif

	cli();

	// Setup timer1 to fire on a 50Hz rate

	// clear all bits on the TCCR1 registers
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;

	// find a proper prescaler for the given frequency

	//OCR1A = (16000000U) / (prescale_divider[prescaler*2] * sample_freq) - 1;
	//uint32_t frequency = 50;
	//uint32_t actual = (16000000U) / (1024U * frequency) - 1;
	//Serial.println(actual);
	//Serial.println(prescale_divider[prescaler*2+1], 16);
	//Serial.println((1<<CS12)|(1<<CS10), 16);
	
	OCR1A = result;

	//OCR1A = (16000000U) / (1024U * frequency) - 1;
	// turn on CTC
	TCCR1B |= (1 << WGM12);
	// Setup prescaler to 1024
	TCCR1B |= (prescale_divider[prescaler*2+1]);
	//TCCR1B |= (1 << CS12) | (1 << CS10);
	// Enable interrupt
	TIMSK1 |= (1 << OCIE1A);
	
	sei();
}

void initialize_player()
{
	init_state();
	init_frequencies();
	init_timer(50);

	// Init profiler pin
	DDRB |= (1<<PB0) | (1<<PB1);

	// set mixer to tone output
	send_data(7, 0xf8);

}

int play_note(int channel, int note, int volume) 
{
	if (channel == -1)
	{
		// use rolling channels
		for (int i = 0; i < 3; ++i) {
			if (!state.channels[i].on) {
				channel = i;
				break;
			}
		}

		// No free channels
		if(channel == -1)
			return channel;
	}

	Channel &chan = state.channels[channel];

	cli();
	chan.note = note;
	chan.freq = frequency(note);
	chan.volume = volume;
	chan.has_instrument = false;
	chan.on = true;
	// make sure mixer is set to output playing channel
	state.mixer_state &= ~(1 << channel);
	sei();

	return channel;
}

int play_note(int channel, int note, struct Instrument &instrument)
{
	if (channel == -1)
	{
		// use rolling channels
		for (int i = 0; i < 3; ++i) {
			if (!state.channels[i].on) {
				channel = i;
				break;
			}
		}

		// No free channels
		if (channel == -1)
			return channel;
	}

	Channel &chan = state.channels[channel];

	cli();
	chan.note = note;
	if(!chan.on)
		chan.instrument = instrument;
	chan.has_instrument = true;
	chan.on = true;
	instrument_note_on(chan.instrument, note);

	// make sure mixer is set to output playing channel
	state.mixer_state &= ~(1 << channel);
	sei();
	return channel;
}

void stop_note(int channel, int note)
{
	Channel &chan = state.channels[channel];
	
	cli();
	if (chan.has_instrument) {
		instrument_note_off(chan.instrument, note);
		if(chan.instrument.arpeggio.steps == 0)
			chan.on = false;
	}
	else {
		chan.freq = 0;
		chan.volume = 0;
		chan.on = false;
		//state.mixer_state |= (1<< channel);
	}
	sei();
}


}
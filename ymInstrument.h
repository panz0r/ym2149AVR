#pragma once
#include <stdint.h>

namespace ym
{


struct ADSR {
	enum { ATTACK, DECAY, SUSTAIN, RELEASE };
};

struct Envelope
{
	uint8_t active : 1;
	uint8_t phase : 2;

	float level;
	float counter;
	float attack;
	float decay;
	float sustain;
	float release;
};

struct Arpeggio
{
	enum Flags
	{
		LOOPING = 0x1,
		BACKWARD = 0x2,
		PINGPONG = 0x4,
	};
	uint8_t active:1;
	uint8_t phase:4;
	uint8_t speed:4;
	uint8_t steps:4;

	uint8_t flags;
	uint8_t counter;
	int8_t offsets[16];
};

struct Instrument
{
	float volume;
	Envelope envelope;
	bool have_arpeggio;
	Arpeggio arpeggio;
};

static Instrument DefaultInstrument = {
	0.f
};

Envelope create_envelope(float a, float d, float s, float r);
float apply_envelope(struct Envelope &env);
int	apply_arpeggio(int base_note, struct Arpeggio &arp);

void apply_instrument(int note, struct Instrument &inst, int &freq, char &level);

void instrument_note_on(struct Instrument &inst, int note);
void instrument_note_off(struct Instrument &inst, int note);

}



#include "ymInstrument.h"
#include "ymFreq.h"
#include <arduino.h>
#if !defined(min)
#define min(a,b) ((a)<(b)?(a):(b))
#endif

namespace ym
{

inline float lerp(float a, float b, float t) {
	return a + t * (b - a);
}


Envelope create_envelope(float a, float d, float s, float r)
{
	float freq_rate = 1.f / (float)_sample_freq;
	 
	Envelope envelope = {
		1,0,0,0,
		a > 0.f ? freq_rate / a : 1.f,
		d > 0.f ? freq_rate / d : 1.f,
		s,
		r > 0.f ? freq_rate / r : 1.f
	};
#if defined(_DEBUG)
	Serial.print("Attack: "); Serial.println(envelope.attack, 6);
	Serial.print("Decay: "); Serial.println(envelope.decay, 6);
	Serial.print("Release: "); Serial.println(envelope.release, 6);
#endif

	return envelope;
}


float apply_envelope(struct Envelope& env)
{
	if (env.active == 0)
		return 0;

	float level = 0.f;
	switch (env.phase) {
	case ADSR::ATTACK:
		level = lerp(0, 1.f, (env.attack >= 1.f ? 1.f : min(1.f, env.counter)));
		env.level = level;
		env.counter += env.attack;
		if (env.counter >= 1.f) {
			env.counter = 0.f;
			env.phase++;
		}
		break;
	case ADSR::DECAY:
		level = lerp(1.f, env.sustain, min(1.f, env.counter));
		env.level = level;
		env.counter += env.decay;
		if (env.counter >= 1.f) {
			env.counter = 0.f;
			env.phase++;
		}
		break;
	case ADSR::SUSTAIN:
		level = env.sustain;
		env.level = level;
		env.counter = 0.f;
		break;
	case ADSR::RELEASE:
		level = lerp(env.level, 0.f, min(1.f, env.counter));
		env.counter += env.release;
		if (env.counter >= 1.f) {
			env.active = 0;
		}
		break;
	}
	return level;
}


int	apply_arpeggio(int base_note, struct Arpeggio &arp)
{
	if (arp.active == 0 || arp.steps < 2)
		return base_note;
	

	int note = arp.offsets[arp.phase];

	int phase = arp.phase;
	int counter = arp.counter;
	counter++;
	if(counter > 255)
		counter = 0;
	arp.counter = counter;

	if ((arp.counter % arp.speed) == 0) {
		phase = phase + ((arp.flags & Arpeggio::BACKWARD) ? -1 : 1);
		if (phase < 0 || phase >= arp.steps) {
			if (arp.flags & Arpeggio::LOOPING) {
		
				// switch direction
				if (arp.flags & Arpeggio::PINGPONG) {
					uint8_t dir = arp.flags & Arpeggio::BACKWARD;
					arp.flags &= ~Arpeggio::BACKWARD;
					arp.flags |= (~dir & Arpeggio::BACKWARD);
					arp.phase = (phase < 0) ? 1 : arp.steps-2;
				}
				else {
					// loop phase
					arp.phase = ((arp.flags & Arpeggio::BACKWARD) ? arp.steps-1 : 0);
				}
			}
			else {
				arp.active = 0;
			}
		}
		else {
			arp.phase = phase;
		}
	}

	return note;
}

void apply_instrument(int note, struct Instrument &inst, int &out_freq, char &out_level)
{
	float level = inst.volume;
	level *= apply_envelope(inst.envelope);
	if(inst.have_arpeggio)
		note = apply_arpeggio(note, inst.arpeggio);
	out_freq = frequency(note);
	out_level = (char)(15 * level);
}

void instrument_note_on(struct Instrument &inst, int note)
{
		inst.envelope.active = 1;
		inst.envelope.counter = 0.f;
		inst.envelope.phase = ADSR::ATTACK;

	if (inst.have_arpeggio) {
		if (inst.arpeggio.steps == 0)
		{
			inst.arpeggio.counter = 0;
			inst.arpeggio.phase = 0;

		}

		int index = inst.arpeggio.steps;
		for (int i = 0; i < inst.arpeggio.steps; ++i) {
			if(note == inst.arpeggio.offsets[i])
				return;

			if (note < inst.arpeggio.offsets[i]) {
				index = i;
				break;
			}
		}

		if (inst.arpeggio.steps > 0) {
			for (int i = inst.arpeggio.steps - 1; i >= index; --i) {
				inst.arpeggio.offsets[i + 1] = inst.arpeggio.offsets[i];
			}
		}

		inst.arpeggio.offsets[index] = note;
		inst.arpeggio.steps++;

		inst.arpeggio.active = 1;
	}
	else {
		inst.envelope.active = 1;
		inst.envelope.counter = 0.f;
		inst.envelope.phase = ADSR::ATTACK;

	}
	
}
void instrument_note_off(struct Instrument &inst, int note)
{

	if (inst.have_arpeggio) {
		int index = -1;
		for (int i = 0; i < inst.arpeggio.steps; ++i) {
			if (inst.arpeggio.offsets[i] == note) {
				index = i;
				break;
			}
		}
		if(index == -1)
			return;

		for (int i = index; i < inst.arpeggio.steps; ++i) {
			inst.arpeggio.offsets[i] = inst.arpeggio.offsets[i+1];
		}
		inst.arpeggio.steps--;
		if (inst.arpeggio.steps == 0)
		{
			if (inst.envelope.active) {
				if (inst.envelope.phase < ADSR::RELEASE)
					inst.envelope.counter = 0.f;
				inst.envelope.phase = ADSR::RELEASE;
			}

		}
	}
	else {
		if (inst.envelope.active) {
			inst.envelope.counter = 0.f;
			inst.envelope.phase = ADSR::RELEASE;
		}

	}

}


}
#include "ymMidi.h"
#include "ymPlay.h"

#if !defined(_DEBUG)
//MIDI_CREATE_DEFAULT_INSTANCE();
#endif

namespace ym
{

bool midi_initialized = false;
int playing_channels[3] = {};

Envelope env1;
Arpeggio arp = 
{
	0,
	0,
	1,
	0,
	Arpeggio::LOOPING,
	0,
	{0}
};
Instrument inst1;

void note_on_cb(byte channel, byte note, byte velocity)
{
	PORTB |= (1<<PB0);

	if (inst1.have_arpeggio) {
		play_note(channel-1, note, inst1);
	}
	else {
		int chan = play_note(-1, note, inst1);
		if (chan != -1) {
			playing_channels[chan] = note;
		}
	}

	PORTB &= ~(1 << PB0);

}

void note_off_cb(byte channel, byte note, byte velocity)
{
	if (inst1.have_arpeggio) {
		stop_note(channel-1, note);
	}
	else {
		for (int i = 0; i < 3; ++i) {
			if (playing_channels[i] == note)
			{
				stop_note(i, note);
				playing_channels[i] = -1;
				break;
			}
		}
	}
}

void program_change_cb(byte channel, byte program)
{
}

void control_change_cb(byte channel, byte number, byte value)
{
}

midi::MidiInterface<HardwareSerial> *_midi = nullptr;

void initialize_midi()
{

	_midi = new midi::MidiInterface<HardwareSerial>((HardwareSerial&)Serial);
	auto &midi = *_midi;

#if !defined(_DEBUG)
	// Register callbacks
	midi.setHandleNoteOn(note_on_cb);
	midi.setHandleNoteOff(note_off_cb);
	midi.setHandleProgramChange(program_change_cb);
	midi.setHandleControlChange(control_change_cb);
	midi.begin(MIDI_CHANNEL_OMNI);
	midi_initialized = true;
#endif

	env1 = ym::create_envelope(0.f, 3.f, 0.f, 0.3f);
	inst1.envelope = env1;
	inst1.have_arpeggio = true;
	inst1.arpeggio = arp;
	inst1.volume = 1.f;
}

void update_midi()
{
#if !defined(_DEBUG)
	if (midi_initialized) {
		_midi->read();
	}
#endif
}


}
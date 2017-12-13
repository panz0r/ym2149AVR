#include "ym2149.h"
#include "ymPlay.h"
#include "ymMidi.h"
#include "ymStreamer.h"

bool _use_midi = false;

void setup()
{

#if defined(_DEBUG)
	//Serial.begin(115200);
	//Serial.println("Starting debug mode");
#endif

	pinMode(13, INPUT);
	_use_midi = digitalRead(13) == 0;

	ym::initialize();
	if (_use_midi) {
		ym::initialize_player();
		ym::initialize_midi();
	}
	else {
		ym::initialize_streamer(57600);
	}
}

void loop()
{
	if (_use_midi) {
		ym::update_midi();
	}
	else {
		ym::update_streamer();
	}
}

#pragma once
#include "ym2149.h"
#include "ymInstrument.h"

namespace ym
{

void initialize_player();

int play_note(int channel, int note, int volume);
int play_note(int channel, int note, struct Instrument &instrument);
void stop_note(int channel, int note);


}
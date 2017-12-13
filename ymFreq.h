#pragma once
#include <stdint.h>

namespace ym
{

extern uint32_t _sample_freq;

// Todo, statically initialize frequencies as PROGMEM
void init_frequencies();
int frequency(int note);


}

#include "portmidi.h"
#include "porttime.h"
#include "stdio.h"
#include "conio.h"
#include <iostream>
#include "portaudio.h"
#include "SynthData.h"
#include "windows.h"
#include <stdlib.h>

using namespace std;

// ------ definicje i deklaracje do PortAudio
static int generateSignal(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData);

// ------ definicje i deklaracje do PortMIDI
#define INPUT_BUFFER_SIZE 100
#define DRIVER_INFO NULL
#define TIME_PROC ((int32_t (*)(void *)) Pt_Time)
#define TIME_INFO NULL
#define NOTE_ON 0x90
#define NOTE_OFF 0x80
PmStream * prepareMIDI();
int chooseMIDIDevice();
PaStream* prepareAudio(synthData &synth);

bool isNoteOn(PmMessage msg);
bool isNoteOff(PmMessage msg);
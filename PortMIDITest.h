#include "portmidi.h"
#include "porttime.h"
#include "stdio.h"
#include "math.h"
#include "conio.h"
#include <iostream>
#include "portaudio.h"
using namespace std;

#ifndef M_PI
	#define M_PI  (3.14159265)
#endif
#define NUM_OF_VOICES 3
#define TABLE_SIZE 44100

float sinus[TABLE_SIZE];
float sawtooth[TABLE_SIZE];
float square[TABLE_SIZE];


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
PmStream * prepareMIDI(int i);

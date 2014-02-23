#include "portmidi.h"
#include "porttime.h"
#include "stdio.h"
#include "conio.h"
#include <iostream>
#include "portaudio.h"
#include "windows.h"
#include <stdlib.h>
#include "WavetableGenerators.h"
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

#define NUM_OF_VOICES 3
#define CIRCLE_BUFFER_SIZE 2
#define FILTER_SIZE 2
#define TONAL_GENERATOR 0
#define NOISE_GENERATOR 1
#define TONAL_GENERATORS_FILTER 2

using namespace std;

// ------ definicje i deklaracje do PortMIDI
#define INPUT_BUFFER_SIZE 100
#define DRIVER_INFO NULL
#define TIME_PROC ((int32_t (*)(void *)) Pt_Time)
#define TIME_INFO NULL
#define NOTE_ON 0x90
#define NOTE_OFF 0x80

// Funkcje przygotowuj�ce obs�ug� MIDI i audio
PmStream * prepareMIDI();
PaStream* prepareAudio();
int chooseMIDIDevice();

// Funkcja pokazuj�ca interfejs tekstowy
bool textInterface();

// Funkcje do obs�ugi zdarze� MIDI
bool isNoteOn(PmMessage msg);
bool isNoteOff(PmMessage msg);

// Funkcje do obs�ugi wyboru g�osu
void setVoiceOn(int note_number, int velocity);
void setVoiceOff(int note_number);
int chooseVoice();
int findVoiceBynote_number(int note_number);

// Funkcja wype�niaj�ca bufor wyj�ciowy karty d�wi�kowej
static int fillOutputBuffer(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData);

// Funkcja generuj�ca nast�pn� pr�bk� sygna�u
float generateNewSample(int voice_number);

// funkcje ustawiaj�ce parametry filtr�w: tonalnego i szumowego
void setTonalFilterParameters(filterType type, float passbandFrequency);
void setNoiseFilterParameters(filterType type, float passbandFrequency);
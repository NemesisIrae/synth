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

// parametry odpowiadaj�ce za brzmienie
float attack_time = 100; // czas ataku [ms]
float release_time = 1000; // czas release [ms]
float sinus_mix = 0;
float sawtooth_mix = 1;
float square_mix = 0;

// ------ definicje i deklaracje do PortAudio
static int generateSignal(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData);
const unsigned short NUM_OF_VOICES = 3;
struct voice
{
	int noteNumber;
	float frequency;
	float gain;
	bool isPlayed;
	int phase = 0;
};
voice voices[NUM_OF_VOICES];
typedef float multiChFloat[NUM_OF_VOICES];
multiChFloat signal;
const unsigned short TABLE_SIZE = 44100;
float sinus[TABLE_SIZE];
float sawtooth[TABLE_SIZE];
float square[TABLE_SIZE];

// ------ definicje i deklaracje do PortMIDI
#define INPUT_BUFFER_SIZE 100
#define DRIVER_INFO NULL
#define TIME_PROC ((int32_t (*)(void *)) Pt_Time)
#define TIME_INFO NULL
#define NOTE_ON 0x90
#define NOTE_OFF 0x80
PmStream * prepareMIDI(int i);

int main(int argc, char *argv[]) {
	int i;

	// deklaracje dla PortMIDI
	PmStream * midiStream;
	PmEvent MIDIbuffer[1];
	int noteNumber;

	// wyb�r urz�dzenia MIDI
	for (i = 0; i < Pm_CountDevices(); i++) {
		const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
		if (info->input) {
			cout << i << ". - " << info->interf << " - " << info->name << "\n";
		}
	}
	cout << "Wybierz urz�dzenie MIDI: ";
	cin >> i;

	// przygotowanie urz�dzenia MIDI
	midiStream = prepareMIDI(i);
	// przygotowanie urz�dzenia audio
	PaStream *audioStream;
	Pa_Initialize();
	Pa_OpenDefaultStream(&audioStream,
						0,
						2,
						paFloat32,
						44100,
						1,
						generateSignal,
						&signal);
	Pa_StartStream(audioStream);
	// wycisz wszystkie g�osy
	for (i = 0; i < NUM_OF_VOICES; i++) {
		voices[i].gain = 0;
		voices[i].isPlayed = 0;
	}
	// przygotowanie tabel z r�nymi sygna�ami
	for (i = 0; i < TABLE_SIZE; i++) {
		sinus[i] = sin(2 * M_PI*double(i) / double(TABLE_SIZE));
		sawtooth[i] = 2*float(i)/float(TABLE_SIZE)-1;
		square[i] = (sinus[i] > 0) - (sinus[i] < 0);
	}
	// g��wna p�tla 
	while (1) {
		if (_kbhit()) 
			// je�li zosta� wci�ni�ty jakikolwiek klawisz na klawiaturze komputerowej - przerwij zabaw�
			break;
		if (Pm_Poll(midiStream)) {
			// odczytaj komunikat MIDI
			Pm_Read(midiStream, MIDIbuffer, 1);
			// zczytaj z tego komunikatu numer nuty MIDI
			noteNumber = Pm_MessageData1(MIDIbuffer[0].message);
			cout << "Message status: " << Pm_MessageStatus(MIDIbuffer[0].message) << ", note number: " << Pm_MessageData1(MIDIbuffer[0].message) << endl;
			// sprawd� czy jest to nuta uderzona
			if (Pm_MessageStatus(MIDIbuffer[0].message) == NOTE_ON) {
				// wybierz pierwszy wolny g�os (dla kt�rego gain == 0)
				i = 0;
				while (voices[i].isPlayed != 0) {
					i++;
				}
				// je�li wszystkie zaj�te, kradniemy pierwszy g�os
				if (i == NUM_OF_VOICES) 
					i = 0;
				// do wybranego g�osu przypisujemy warto�ci wzi�te z komunikatu MIDI
				voices[i].frequency = pow(2, double(noteNumber - 69.0f) / 12.0f) * 440.0f;
				cout << "   frequency: " << voices[i].frequency << endl;
				voices[i].isPlayed = 1;
				voices[i].noteNumber = noteNumber;
			}
			// je�li nie jest uderzona to sprawd� czy jest puszczona
			else if (Pm_MessageStatus(MIDIbuffer[0].message) == NOTE_OFF) {
				// znajd� g�os, kt�rego noteNumber jest taki sam jak w�a�nie puszczony klawisz
				i = 0;
				while ((voices[i].noteNumber != noteNumber) && i<NUM_OF_VOICES) {
					i++;
				}
				// je�li ten g�os jest akurat grany, wycisz go
				if (i < NUM_OF_VOICES) {
					voices[i].isPlayed = 0;
				}
			}
		}
	}
	Pa_StopStream(audioStream);
	// zamkni�cie urz�dzenia MIDI
	Pm_Close(midiStream);
	// zamkni�cie urz�dzenia Audio
	Pa_CloseStream(audioStream);
	Pa_Terminate();

	return 0;
}
PmStream* prepareMIDI(int i) {
	PmEvent MIDIbuffer[1];
	PmStream* midiStream;

	Pt_Start(1, 0, 0);
	Pm_OpenInput(&midiStream,
				i,
				DRIVER_INFO,
				INPUT_BUFFER_SIZE,
				TIME_PROC,
				TIME_INFO);
	Pm_SetFilter(midiStream, PM_FILT_ACTIVE | PM_FILT_CLOCK | PM_FILT_SYSEX);
	while (Pm_Poll(midiStream)) {
		Pm_Read(midiStream, MIDIbuffer, 1);
	}
	return midiStream;
}
static int generateSignal(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData)
{
	// funkcja generuje sygna� i przekazuje go do bufora wyj�ciowego
	multiChFloat* signal = (multiChFloat*)userData;
	float *out = (float*)outputBuffer;
	//--------------
	unsigned int i, j, q;
	float delta[NUM_OF_VOICES];
	for (i = 0; i < framesPerBuffer; i++)
	{
		for (j = 0; j < 2; j++) { // p�tla po kana�ach (w tym przypadku dw�ch: lewym i prawym)
			*out = 0;
			for (q = 0; q < NUM_OF_VOICES; q++) {
				// zaktualizuj gain kolejnego g�osu
				// attack
				if (voices[q].isPlayed && (voices[q].gain < 1))
					voices[q].gain += 1 / (attack_time*44.100f);
				// release
				if (!voices[q].isPlayed && (voices[q].gain > 0))
					voices[q].gain -= 1 / (release_time*44.100f);
				
				// do warto�ci pr�bki w buforze wyj�ciowym dodaj warto�� pr�bki sygna�u generowanego przez q-ty g�os
				*out = *out + *signal[q]*voices[q].gain;
			}
			// przesu� wska�nik na kolejn� pr�bk� w buforze wyj�ciowym
			out++;
		}

		for (q = 0; q < NUM_OF_VOICES; q++) {
			// oblicz delt� (czyli jak du�y jest krok pomi�dzy odczytywanymi pr�bkami z tabeli - im wi�kszy tym wi�ksza cz�stotliwo��)
			delta[q] = voices[q].frequency;
			// zaktualizuj sygna� generowany przez kolejny g�os o zmiksowane sygna�y z generator�w sinusa, pi�y i prostok�ta
			*signal[q] = (sinus_mix*sinus[voices[q].phase] + sawtooth_mix*sawtooth[voices[q].phase] + square_mix*square[voices[q].phase]) / 4;
			
			voices[q].phase += delta[q];
			if (voices[q].phase > TABLE_SIZE)
				voices[q].phase = 0;
		}
	}
	return 0;
}
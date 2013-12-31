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

// ------ definicje i deklaracje do PortAudio
static int generateSignal(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData);
const unsigned short NUM_OF_VOICES = 3;
typedef float multiChFloat[NUM_OF_VOICES];
const unsigned short TABLE_SIZE = 44100;
float sinus[TABLE_SIZE];
float sawtooth[TABLE_SIZE];
float square[TABLE_SIZE];

struct voice
{
	int noteNumber;
	float frequency;
	float gain;
	bool isPlayed;
	int phase = 0;
};
struct synthParameters //P: typ trzymaj�cy parametry syntezatora
{
	// parametry odpowiadaj�ce za brzmienie
	float attack_time = 100; // czas ataku [ms]
	float release_time = 1000; // czas release [ms]
	float sinus_mix = 0;
	float sawtooth_mix = 1;
	float square_mix = 0;
};

struct mainData // P: typ trzymaj�cy g��wne parametry przesy�ane do generateSignal
{
	synthParameters parameters;
	multiChFloat signal;
	voice voices[3];
	//mainData(synthParameters, multiChFloat*, voice(*)[3]);
};


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

	// P: wprowadzenie parametr�w syntezatora
	//voice xMasVoice[NUM_OF_VOICES];
	//synthParameters xMasParameters; // P: parametry dla wybranego syntezatora; tutaj xMasSynth ;)
	//multiChFloat xMasSignal; // P: wielo-voice'owy sygna� dla wybranego syntezatora
	mainData xMasSynth; // P: parametry xMasSyth'a; paczka dla generateSignal 
	//xMasSynth.parameters = xMasParameters;
	//*xMasSynth.voices = &xMasVoice;
	//xMasSynth.signal = xMasSignal;
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
						&xMasSynth);
	Pa_StartStream(audioStream);
	// wycisz wszystkie g�osy
	for (i = 0; i < NUM_OF_VOICES; i++) {
		xMasSynth.voices[i].gain = 0;
		xMasSynth.voices[i].isPlayed = 0;
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
				while (xMasSynth.voices[i].isPlayed != 0) {
					i++;
				}
				// je�li wszystkie zaj�te, kradniemy pierwszy g�os
				if (i == NUM_OF_VOICES) 
					i = 0;
				// do wybranego g�osu przypisujemy warto�ci wzi�te z komunikatu MIDI
				xMasSynth.voices[i].frequency = pow(2, double(noteNumber - 69.0f) / 12.0f) * 440.0f;
				cout << "   frequency: " << xMasSynth.voices[i].frequency << endl;
				xMasSynth.voices[i].isPlayed = 1;
				xMasSynth.voices[i].noteNumber = noteNumber;
			}
			// je�li nie jest uderzona to sprawd� czy jest puszczona
			else if (Pm_MessageStatus(MIDIbuffer[0].message) == NOTE_OFF) {
				// znajd� g�os, kt�rego noteNumber jest taki sam jak w�a�nie puszczony klawisz
				i = 0;
				while ((xMasSynth.voices[i].noteNumber != noteNumber) && i<NUM_OF_VOICES) {
					i++;
				}
				// je�li ten g�os jest akurat grany, wycisz go
				if (i < NUM_OF_VOICES) {
					xMasSynth.voices[i].isPlayed = 0;
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
	mainData* synthData = (mainData*)userData;
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
				if ((*synthData).voices[q].isPlayed && ((*synthData).voices[q].gain < 1))
					(*synthData).voices[q].gain += 1 / ((*synthData).parameters.attack_time*44.100f);
				// release
				if (!(*synthData).voices[q].isPlayed && ((*synthData).voices[q].gain > 0))
					(*synthData).voices[q].gain -= 1 / ((*synthData).parameters.release_time*44.100f);
				
				// do warto�ci pr�bki w buforze wyj�ciowym dodaj warto�� pr�bki sygna�u generowanego przez q-ty g�os
				*out = *out + (*synthData).signal[q] * (*synthData).voices[q].gain;
			}
			// przesu� wska�nik na kolejn� pr�bk� w buforze wyj�ciowym
			out++;
		}

		for (q = 0; q < NUM_OF_VOICES; q++) {
			// oblicz delt� (czyli jak du�y jest krok pomi�dzy odczytywanymi pr�bkami z tabeli - im wi�kszy tym wi�ksza cz�stotliwo��)
			delta[q] = (*synthData).voices[q].frequency;
			// zaktualizuj sygna� generowany przez kolejny g�os o zmiksowane sygna�y z generator�w sinusa, pi�y i prostok�ta
			(*synthData).signal[q] = ((*synthData).parameters.sinus_mix*sinus[(*synthData).voices[q].phase] +
									  (*synthData).parameters.sawtooth_mix*sawtooth[(*synthData).voices[q].phase] +
									  (*synthData).parameters.square_mix*square[(*synthData).voices[q].phase]) / 4;
			
			(*synthData).voices[q].phase += delta[q];
			if ((*synthData).voices[q].phase > TABLE_SIZE)
				(*synthData).voices[q].phase = 0;
		}
	}
	return 0;
}
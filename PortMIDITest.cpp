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

// parametry odpowiadaj¹ce za brzmienie
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

	// wybór urz¹dzenia MIDI
	for (i = 0; i < Pm_CountDevices(); i++) {
		const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
		if (info->input) {
			cout << i << ". - " << info->interf << " - " << info->name << "\n";
		}
	}
	cout << "Wybierz urz¹dzenie MIDI: ";
	cin >> i;

	// przygotowanie urz¹dzenia MIDI
	midiStream = prepareMIDI(i);
	// przygotowanie urz¹dzenia audio
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
	// wycisz wszystkie g³osy
	for (i = 0; i < NUM_OF_VOICES; i++) {
		voices[i].gain = 0;
		voices[i].isPlayed = 0;
	}
	// przygotowanie tabel z ró¿nymi sygna³ami
	for (i = 0; i < TABLE_SIZE; i++) {
		sinus[i] = sin(2 * M_PI*double(i) / double(TABLE_SIZE));
		sawtooth[i] = 2*float(i)/float(TABLE_SIZE)-1;
		square[i] = (sinus[i] > 0) - (sinus[i] < 0);
	}
	// g³ówna pêtla 
	while (1) {
		if (_kbhit()) 
			// jeœli zosta³ wciœniêty jakikolwiek klawisz na klawiaturze komputerowej - przerwij zabawê
			break;
		if (Pm_Poll(midiStream)) {
			// odczytaj komunikat MIDI
			Pm_Read(midiStream, MIDIbuffer, 1);
			// zczytaj z tego komunikatu numer nuty MIDI
			noteNumber = Pm_MessageData1(MIDIbuffer[0].message);
			cout << "Message status: " << Pm_MessageStatus(MIDIbuffer[0].message) << ", note number: " << Pm_MessageData1(MIDIbuffer[0].message) << endl;
			// sprawdŸ czy jest to nuta uderzona
			if (Pm_MessageStatus(MIDIbuffer[0].message) == NOTE_ON) {
				// wybierz pierwszy wolny g³os (dla którego gain == 0)
				i = 0;
				while (voices[i].isPlayed != 0) {
					i++;
				}
				// jeœli wszystkie zajête, kradniemy pierwszy g³os
				if (i == NUM_OF_VOICES) 
					i = 0;
				// do wybranego g³osu przypisujemy wartoœci wziête z komunikatu MIDI
				voices[i].frequency = pow(2, double(noteNumber - 69.0f) / 12.0f) * 440.0f;
				cout << "   frequency: " << voices[i].frequency << endl;
				voices[i].isPlayed = 1;
				voices[i].noteNumber = noteNumber;
			}
			// jeœli nie jest uderzona to sprawdŸ czy jest puszczona
			else if (Pm_MessageStatus(MIDIbuffer[0].message) == NOTE_OFF) {
				// znajdŸ g³os, którego noteNumber jest taki sam jak w³aœnie puszczony klawisz
				i = 0;
				while ((voices[i].noteNumber != noteNumber) && i<NUM_OF_VOICES) {
					i++;
				}
				// jeœli ten g³os jest akurat grany, wycisz go
				if (i < NUM_OF_VOICES) {
					voices[i].isPlayed = 0;
				}
			}
		}
	}
	Pa_StopStream(audioStream);
	// zamkniêcie urz¹dzenia MIDI
	Pm_Close(midiStream);
	// zamkniêcie urz¹dzenia Audio
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
	// funkcja generuje sygna³ i przekazuje go do bufora wyjœciowego
	multiChFloat* signal = (multiChFloat*)userData;
	float *out = (float*)outputBuffer;
	//--------------
	unsigned int i, j, q;
	float delta[NUM_OF_VOICES];
	for (i = 0; i < framesPerBuffer; i++)
	{
		for (j = 0; j < 2; j++) { // pêtla po kana³ach (w tym przypadku dwóch: lewym i prawym)
			*out = 0;
			for (q = 0; q < NUM_OF_VOICES; q++) {
				// zaktualizuj gain kolejnego g³osu
				// attack
				if (voices[q].isPlayed && (voices[q].gain < 1))
					voices[q].gain += 1 / (attack_time*44.100f);
				// release
				if (!voices[q].isPlayed && (voices[q].gain > 0))
					voices[q].gain -= 1 / (release_time*44.100f);
				
				// do wartoœci próbki w buforze wyjœciowym dodaj wartoœæ próbki sygna³u generowanego przez q-ty g³os
				*out = *out + *signal[q]*voices[q].gain;
			}
			// przesuñ wskaŸnik na kolejn¹ próbkê w buforze wyjœciowym
			out++;
		}

		for (q = 0; q < NUM_OF_VOICES; q++) {
			// oblicz deltê (czyli jak du¿y jest krok pomiêdzy odczytywanymi próbkami z tabeli - im wiêkszy tym wiêksza czêstotliwoœæ)
			delta[q] = voices[q].frequency;
			// zaktualizuj sygna³ generowany przez kolejny g³os o zmiksowane sygna³y z generatorów sinusa, pi³y i prostok¹ta
			*signal[q] = (sinus_mix*sinus[voices[q].phase] + sawtooth_mix*sawtooth[voices[q].phase] + square_mix*square[voices[q].phase]) / 4;
			
			voices[q].phase += delta[q];
			if (voices[q].phase > TABLE_SIZE)
				voices[q].phase = 0;
		}
	}
	return 0;
}
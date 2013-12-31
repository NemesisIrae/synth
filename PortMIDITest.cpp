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
struct synthParameters //P: typ trzymaj¹cy parametry syntezatora
{
	// parametry odpowiadaj¹ce za brzmienie
	float attack_time = 100; // czas ataku [ms]
	float release_time = 1000; // czas release [ms]
	float sinus_mix = 0;
	float sawtooth_mix = 1;
	float square_mix = 0;
};

struct mainData // P: typ trzymaj¹cy g³ówne parametry przesy³ane do generateSignal
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

	// P: wprowadzenie parametrów syntezatora
	//voice xMasVoice[NUM_OF_VOICES];
	//synthParameters xMasParameters; // P: parametry dla wybranego syntezatora; tutaj xMasSynth ;)
	//multiChFloat xMasSignal; // P: wielo-voice'owy sygna³ dla wybranego syntezatora
	mainData xMasSynth; // P: parametry xMasSyth'a; paczka dla generateSignal 
	//xMasSynth.parameters = xMasParameters;
	//*xMasSynth.voices = &xMasVoice;
	//xMasSynth.signal = xMasSignal;
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
						&xMasSynth);
	Pa_StartStream(audioStream);
	// wycisz wszystkie g³osy
	for (i = 0; i < NUM_OF_VOICES; i++) {
		xMasSynth.voices[i].gain = 0;
		xMasSynth.voices[i].isPlayed = 0;
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
				while (xMasSynth.voices[i].isPlayed != 0) {
					i++;
				}
				// jeœli wszystkie zajête, kradniemy pierwszy g³os
				if (i == NUM_OF_VOICES) 
					i = 0;
				// do wybranego g³osu przypisujemy wartoœci wziête z komunikatu MIDI
				xMasSynth.voices[i].frequency = pow(2, double(noteNumber - 69.0f) / 12.0f) * 440.0f;
				cout << "   frequency: " << xMasSynth.voices[i].frequency << endl;
				xMasSynth.voices[i].isPlayed = 1;
				xMasSynth.voices[i].noteNumber = noteNumber;
			}
			// jeœli nie jest uderzona to sprawdŸ czy jest puszczona
			else if (Pm_MessageStatus(MIDIbuffer[0].message) == NOTE_OFF) {
				// znajdŸ g³os, którego noteNumber jest taki sam jak w³aœnie puszczony klawisz
				i = 0;
				while ((xMasSynth.voices[i].noteNumber != noteNumber) && i<NUM_OF_VOICES) {
					i++;
				}
				// jeœli ten g³os jest akurat grany, wycisz go
				if (i < NUM_OF_VOICES) {
					xMasSynth.voices[i].isPlayed = 0;
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
	mainData* synthData = (mainData*)userData;
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
				if ((*synthData).voices[q].isPlayed && ((*synthData).voices[q].gain < 1))
					(*synthData).voices[q].gain += 1 / ((*synthData).parameters.attack_time*44.100f);
				// release
				if (!(*synthData).voices[q].isPlayed && ((*synthData).voices[q].gain > 0))
					(*synthData).voices[q].gain -= 1 / ((*synthData).parameters.release_time*44.100f);
				
				// do wartoœci próbki w buforze wyjœciowym dodaj wartoœæ próbki sygna³u generowanego przez q-ty g³os
				*out = *out + (*synthData).signal[q] * (*synthData).voices[q].gain;
			}
			// przesuñ wskaŸnik na kolejn¹ próbkê w buforze wyjœciowym
			out++;
		}

		for (q = 0; q < NUM_OF_VOICES; q++) {
			// oblicz deltê (czyli jak du¿y jest krok pomiêdzy odczytywanymi próbkami z tabeli - im wiêkszy tym wiêksza czêstotliwoœæ)
			delta[q] = (*synthData).voices[q].frequency;
			// zaktualizuj sygna³ generowany przez kolejny g³os o zmiksowane sygna³y z generatorów sinusa, pi³y i prostok¹ta
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
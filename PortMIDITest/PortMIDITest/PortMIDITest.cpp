#include "PortMIDITest.h"
float sinus[TABLE_SIZE];
float sawtooth[TABLE_SIZE];
float square[TABLE_SIZE];
int main(int argc, char *argv[]) {
	int i;
	//float* sawtooth[TABLE_SIZE];
	//float* square[TABLE_SIZE];
	GenerateSineTable(sinus);
	GenerateSawtoothTable(sawtooth);
	GenerateSquareTable(square);
	//GenerateSawtoothTable(*sawtooth);
	//GenerateSquareTable(*square);
	// P: wprowadzenie parametrów syntezatora
	synthData xMasSynth; // P: parametry xMasSyth'a ;) - paczka parametrów dla generateSignal 
	xMasSynth.mix.sin = 1;
	// deklaracje dla PortMIDI
	PmStream * midiStream;
	PmEvent MIDIbuffer[1];
	int noteNumber;

	// wybór urz¹dzenia MIDI
	for (i = 0; i < Pm_CountDevices(); i++) 
	{
		const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
		if (info->input) 
		{
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
						128,
						generateSignal,
						&xMasSynth);
	Pa_StartStream(audioStream);
	// wycisz wszystkie g³osy
	for (i = 0; i < NUM_OF_VOICES; i++) {
		xMasSynth.voice[i].gain = 0;
		xMasSynth.voice[i].isPlayed = 0;
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
				while (xMasSynth.voice[i].isPlayed != 0) {
					i++;
				}
				// jeœli wszystkie zajête, kradniemy pierwszy g³os
				if (i == NUM_OF_VOICES) 
					i = 0;
				// do wybranego g³osu przypisujemy wartoœci wziête z komunikatu MIDI
				xMasSynth.voice[i].frequency = pow(2, double(noteNumber - 69.0f) / 12.0f) * 440.0f;
				cout << "   frequency: " << xMasSynth.voice[i].frequency << endl;
				xMasSynth.voice[i].isPlayed = 1;
				xMasSynth.voice[i].noteNumber = noteNumber;
			}
			// jeœli nie jest uderzona to sprawdŸ czy jest puszczona
			else if (Pm_MessageStatus(MIDIbuffer[0].message) == NOTE_OFF) {
				// znajdŸ g³os, którego noteNumber jest taki sam jak w³aœnie puszczony klawisz
				i = 0;
				while ((xMasSynth.voice[i].noteNumber != noteNumber) && i<NUM_OF_VOICES) {
					i++;
				}
				// jeœli ten g³os jest akurat grany, wycisz go
				if (i < NUM_OF_VOICES) {
					xMasSynth.voice[i].isPlayed = 0;
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
	synthData* synth = (synthData*)userData;
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
				if ((*synth).voice[q].isPlayed && ((*synth).voice[q].gain < 1))
					(*synth).voice[q].gain += 1 / ((*synth).adsr.attack*44.100f);
				// release
				if (!(*synth).voice[q].isPlayed && ((*synth).voice[q].gain > 0))
					(*synth).voice[q].gain -= 1 / ((*synth).adsr.release*44.100f);
				
				// do wartoœci próbki w buforze wyjœciowym dodaj wartoœæ próbki sygna³u generowanego przez q-ty g³os
				*out = *out + (*synth).signal[q] * (*synth).voice[q].gain;
			}
			// przesuñ wskaŸnik na kolejn¹ próbkê w buforze wyjœciowym
			out++;
		}

		for (q = 0; q < NUM_OF_VOICES; q++) {
			// oblicz deltê (czyli jak du¿y jest krok pomiêdzy odczytywanymi próbkami z tabeli - im wiêkszy tym wiêksza czêstotliwoœæ)
			delta[q] = (*synth).voice[q].frequency;
			// zaktualizuj sygna³ generowany przez kolejny g³os o zmiksowane sygna³y z generatorów sinusa, pi³y i prostok¹ta
			(*synth).signal[q] = ((*synth).mix.sin * sinus[(*synth).voice[q].phase] +
									  (*synth).mix.saw * sawtooth[(*synth).voice[q].phase] +
									  (*synth).mix.sqr * square[(*synth).voice[q].phase]) / 4;
			
			(*synth).voice[q].phase += delta[q];
			if ((*synth).voice[q].phase > TABLE_SIZE)
				(*synth).voice[q].phase = 0;
		}
	}
	return 0;
}
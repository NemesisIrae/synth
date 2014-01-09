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
	// P: wprowadzenie parametr�w syntezatora
	synthData xMasSynth; // P: parametry xMasSyth'a ;) - paczka parametr�w dla generateSignal 
	xMasSynth.mix.sin = 1;
	// deklaracje dla PortMIDI
	PmStream * midiStream;
	PmEvent MIDIbuffer[1];
	int noteNumber;

	// wyb�r urz�dzenia MIDI
	for (i = 0; i < Pm_CountDevices(); i++) 
	{
		const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
		if (info->input) 
		{
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
						128,
						generateSignal,
						&xMasSynth);
	Pa_StartStream(audioStream);
	// wycisz wszystkie g�osy
	for (i = 0; i < NUM_OF_VOICES; i++) {
		xMasSynth.voice[i].gain = 0;
		xMasSynth.voice[i].isPlayed = 0;
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
				while (xMasSynth.voice[i].isPlayed != 0) {
					i++;
				}
				// je�li wszystkie zaj�te, kradniemy pierwszy g�os
				if (i == NUM_OF_VOICES) 
					i = 0;
				// do wybranego g�osu przypisujemy warto�ci wzi�te z komunikatu MIDI
				xMasSynth.voice[i].frequency = pow(2, double(noteNumber - 69.0f) / 12.0f) * 440.0f;
				cout << "   frequency: " << xMasSynth.voice[i].frequency << endl;
				xMasSynth.voice[i].isPlayed = 1;
				xMasSynth.voice[i].noteNumber = noteNumber;
			}
			// je�li nie jest uderzona to sprawd� czy jest puszczona
			else if (Pm_MessageStatus(MIDIbuffer[0].message) == NOTE_OFF) {
				// znajd� g�os, kt�rego noteNumber jest taki sam jak w�a�nie puszczony klawisz
				i = 0;
				while ((xMasSynth.voice[i].noteNumber != noteNumber) && i<NUM_OF_VOICES) {
					i++;
				}
				// je�li ten g�os jest akurat grany, wycisz go
				if (i < NUM_OF_VOICES) {
					xMasSynth.voice[i].isPlayed = 0;
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
	synthData* synth = (synthData*)userData;
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
				if ((*synth).voice[q].isPlayed && ((*synth).voice[q].gain < 1))
					(*synth).voice[q].gain += 1 / ((*synth).adsr.attack*44.100f);
				// release
				if (!(*synth).voice[q].isPlayed && ((*synth).voice[q].gain > 0))
					(*synth).voice[q].gain -= 1 / ((*synth).adsr.release*44.100f);
				
				// do warto�ci pr�bki w buforze wyj�ciowym dodaj warto�� pr�bki sygna�u generowanego przez q-ty g�os
				*out = *out + (*synth).signal[q] * (*synth).voice[q].gain;
			}
			// przesu� wska�nik na kolejn� pr�bk� w buforze wyj�ciowym
			out++;
		}

		for (q = 0; q < NUM_OF_VOICES; q++) {
			// oblicz delt� (czyli jak du�y jest krok pomi�dzy odczytywanymi pr�bkami z tabeli - im wi�kszy tym wi�ksza cz�stotliwo��)
			delta[q] = (*synth).voice[q].frequency;
			// zaktualizuj sygna� generowany przez kolejny g�os o zmiksowane sygna�y z generator�w sinusa, pi�y i prostok�ta
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
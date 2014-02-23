#include "PortMIDITest.h"

// INICJALIZACJA TABLICY GENERATOR�W TONALNYCH
tonalGeneratorParameters tonalGenerators[3];
// INICJALIZACJA GENERATORA SZUMOWEGO
noiseGeneretorParameters noiseGenerator;
// INICJALIZACJA TABLICY G�OS�W
voice voices[NUM_OF_VOICES];

int main() {
	PmStream * midiStream;
	PmEvent MIDIbuffer[1];
	int note_number, velocity;
	srand(time(NULL));

	// USTAWIENIE PARAMETR�W SYNTEZATORA
	// generatory tonalne
	tonalGenerators[0].chooseSignalType(SINUS);
	tonalGenerators[1].chooseSignalType(SAWTOOTH);
	tonalGenerators[2].chooseSignalType(SQUARE);
	tonalGenerators[0].setGain(.15f);
	tonalGenerators[1].setGain(.1f);
	tonalGenerators[2].setGain(.2f);
	tonalGenerators[0].setDetune(.0f);
	tonalGenerators[1].setDetune(-.1f);
	tonalGenerators[2].setDetune(.1f);
	// generator szumu
	noiseGenerator.setGain(.2);
	// ustawienie parametr�w ADSR
	adsrs[TONAL_GENERATOR].setAttack(10);
	adsrs[TONAL_GENERATOR].setRelease(1000);
	adsrs[NOISE_GENERATOR].setAttack(5);
	adsrs[NOISE_GENERATOR].setRelease(10);

	// parametry filtr�w, mog� by� podane przez interface
	filterType tonalFilterType, noiseFilterType;
	float tonalPassbandFrequency, noisePassbandFrequency;
	tonalFilterType = lowPass;
	noiseFilterType = lowPass;
	tonalPassbandFrequency = 1000.0f;
	noisePassbandFrequency = 400.0f;

	setTonalFilterParameters(tonalFilterType, tonalPassbandFrequency);
	setNoiseFilterParameters(noiseFilterType, noisePassbandFrequency);

	// przygotowanie urz�dzenia MIDI
	midiStream = prepareMIDI();
	// przygotowanie urz�dzenia audio
	PaStream *audioStream = prepareAudio();

	// G��WNA P�TLA 
	while (1) {
		if (_kbhit())
			if (textInterface()) break;
		if (Pm_Poll(midiStream)) {
			// odczytaj komunikat MIDI
			Pm_Read(midiStream, MIDIbuffer, 1);
			// zczytaj z tego komunikatu numer nuty MIDI
			note_number = Pm_MessageData1(MIDIbuffer[0].message);
			velocity = Pm_MessageData2(MIDIbuffer[0].message);
			// cout << "Message status: " << Pm_MessageStatus(MIDIbuffer[0].message) << ", note number: " << Pm_MessageData1(MIDIbuffer[0].message) << endl;
			// sprawd� czy jest to nuta uderzona
			if (isNoteOn(MIDIbuffer[0].message)) {
				// w��cz g�os i przeka� mu numer d�wi�ku MIDI 
				setVoiceOn(note_number, velocity);
			}
			// je�li nie jest uderzona to sprawd� czy jest puszczona
			else if (isNoteOff(MIDIbuffer[0].message)) {
				// wy��cz g�os, kt�ry gra t� nut�
				setVoiceOff(note_number);
			}
		}
	}
	// zamkni�cie urz�dzenia MIDI
	Pm_Close(midiStream);

	// zamkni�cie urz�dzenia Audio
	Pa_StopStream(audioStream);
	Pa_CloseStream(audioStream);
	Pa_Terminate();
	return 0;
}

// Funkcje przygotowuj�ce obs�ug� MIDI i audio
PmStream* prepareMIDI() {

	int device_number = chooseMIDIDevice();

	PmEvent MIDIbuffer[1];
	PmStream* midiStream = 0;

	Pt_Start(1, 0, 0);
	Pm_OpenInput(&midiStream,
				device_number,
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
PaStream* prepareAudio() {
	PaStream *audioStream;
	Pa_Initialize();
	Pa_OpenDefaultStream(&audioStream,
		0,
		2,
		paFloat32,
		44100,
		128,
		fillOutputBuffer,
		0);
	Pa_StartStream(audioStream);

	return audioStream;
}
int chooseMIDIDevice() {
	int MIDI_device_number;
	for (MIDI_device_number = 0; MIDI_device_number < Pm_CountDevices(); MIDI_device_number++) {
		const PmDeviceInfo *info = Pm_GetDeviceInfo(MIDI_device_number);
		if (info->input) {
			cout << MIDI_device_number << ". - " << info->interf << " - " << info->name << "\n";
		}
	}
	cout << "Wybierz urz�dzenie MIDI: ";
	cin >> MIDI_device_number;

	return MIDI_device_number;
}

// Funkcje do obs�ugi zdarze� MIDI
bool isNoteOn(PmMessage msg) {	return (Pm_MessageStatus(msg) == NOTE_ON) && Pm_MessageData2(msg);}
bool isNoteOff(PmMessage msg) {	return (Pm_MessageStatus(msg) == NOTE_OFF) || !Pm_MessageData2(msg);}

// Funkcje do obs�ugi wyboru g�osu
void setVoiceOn(int note_number, int velocity) {
	int voice_number = chooseVoice();
	voices[voice_number].frequency = pow(2, float(note_number - 69.0f) / 12.0f) * 440.0f;
	voices[voice_number].isPlayed = 1;
	voices[voice_number].note_number = note_number;
}
void setVoiceOff(int note_number) {
	int voice_number = findVoiceBynote_number(note_number);
	if (voice_number < NUM_OF_VOICES) {
		voices[voice_number].isPlayed = 0;
	}
}
int chooseVoice() {
	// funkcja zwraca numer pierwszego wolnego g�osu. Je�li wszystkie s� zaj�te, zwraca numer pierwszego g�osu.
	int voice_number = 0;
	while (voices[voice_number].isPlayed != 0) {
		voice_number++;
	}
	// je�li wszystkie zaj�te, kradniemy pierwszy g�os
	if (voice_number == NUM_OF_VOICES)
		voice_number = 0;
	return voice_number;
}
int findVoiceBynote_number(int note_number) {
	int voice_number = 0;
	while ((voices[voice_number].note_number != note_number) && voice_number < NUM_OF_VOICES) {
		voice_number++;
	}

	return voice_number;
}

// Funkcja wype�niaj�ca bufor wyj�ciowy karty d�wi�kowej
static int fillOutputBuffer(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData)
{
	float *out = (float*)outputBuffer;
	unsigned int frame_number, voice_number;

	for (frame_number = 0; frame_number < framesPerBuffer; frame_number++)
	{
		*out = 0;
		// Zsumuj g�osy i przepisz je do pierwszego kana�u bufora wyj�ciowego
		for (voice_number = 0; voice_number < NUM_OF_VOICES; voice_number++)
			*out = *out + generateNewSample(voice_number);
		
		// Identyczn� warto�� pr�bki wpisz do drugiego kana�u
		*(++out) = *(out-1);
		// przesu� wska�nik na kolejn� pr�bk� w buforze wyj�ciowym
		out++;
	}
	return 0;
}

// Funkcja generuj�ca nast�pn� pr�bk� sygna�u
// TODO: Filtry
float generateNewSample(int voice_number) {
	float tonal_sample = 0;
	float noise_sample = 0;
	float out_sample = 0;
	float delta;

	// 1. obliczenie pr�bki z generator�w tonalnych (sumowanie sygna��w z kolejnych generator�w)
	for (int generator_number = 0; generator_number < 3; generator_number++) {
		tonal_sample += tonalGenerators[generator_number].wavetable[voices[voice_number].phases[generator_number]] * tonalGenerators[generator_number].gain;

		// obliczenie o ile przesun�� "faz�" - w oparciu o numer nuty MIDI danego g�osu oraz odstrojenie danego generatora
		delta = pow(2, float(voices[voice_number].note_number + tonalGenerators[generator_number].detune - 69.0f) / 12.0f) * 440.0f;
		// przesuni�cie tej "fazy"
		// UWAGA! 
		// a) chyba dobrym pomys�em b�dzie zmiana tej zmiennej "phase" na co� co lepiej t�umaczy jej dzia�anie. Ale na razie nie mam pomys�u na jak� ;) 
		// b) wyrzuci�em na razie modulacj� FM. Ale to si� zrobi p�niej :) 
		voices[voice_number].phases[generator_number] =
			(voices[voice_number].phases[generator_number] + int(delta + .5)) % TABLE_SIZE;
	}

	// 2. Obliczenie pr�bki pochodz�cej z generatora szumu
	noise_sample = float(rand() % 100)/100 * noiseGenerator.gain;

	// 3. Obliczanie gain g�osu (w oparciu o ADSR) dla pr�bek pochodz�cych z generatora tonalnego oraz szumowego
	voices[voice_number].updateGain(TONAL_GENERATOR);
	tonal_sample *= voices[voice_number].gain[TONAL_GENERATOR];
	voices[voice_number].tonalFilter.updateInSample(tonal_sample); // wstawienie aktualnej pr�bki do bufora pr�bek wej�ciowych filtru tonalnego

	voices[voice_number].updateGain(NOISE_GENERATOR);
	noise_sample *= voices[voice_number].gain[NOISE_GENERATOR];
	voices[voice_number].noiseFilter.updateInSample(noise_sample);

	// 4. Sumowanie pr�bek sygna�u tonalnego i szumowego i wpisanie aktualnej pr�bki do bufora poprzednich pr�bek wej�ciowych filtru


	// 5. Filtracja 
	tonal_sample = voices[voice_number].tonalFilter.Filtrate();
	noise_sample = voices[voice_number].noiseFilter.Filtrate();
		// wpisanie aktualnej pr�bki do bufora poprzednich pr�bek wyj�ciowych filtru
	voices[voice_number].tonalFilter.updateOutSample(tonal_sample);
	voices[voice_number].noiseFilter.updateOutSample(noise_sample);

	return tonal_sample + noise_sample;
}

bool textInterface() {
	char letter;
	cout << "Press q to quit: ";
	cin >> letter;
	if (letter == 'q')
		return 1;
	else
		return 0;
}

// funkcje ustawiaj�ce parametry filtr�w: tonalnego i szumowego
void setTonalFilterParameters(filterType type, float passbandFrequency)
{
	for (int i = 0; i < NUM_OF_VOICES; i++)
	{
		voices[i].tonalFilter.setFilterType(type);
		voices[i].tonalFilter.setPassBandFrequency(passbandFrequency);
	}
}

void setNoiseFilterParameters(filterType type, float passbandFrequency)
{
	for (int i = 0; i < NUM_OF_VOICES; i++)
	{
		voices[i].noiseFilter.setFilterType(type);
		voices[i].noiseFilter.setPassBandFrequency(passbandFrequency);
	}
}
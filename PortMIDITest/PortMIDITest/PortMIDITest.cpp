#include "PortMIDITest.h"

// INICJALIZACJA TABLICY GENERATORÓW TONALNYCH
tonalGeneratorParameters tonalGenerators[3];
// INICJALIZACJA GENERATORA SZUMOWEGO
noiseGeneretorParameters noiseGenerator;
// INICJALIZACJA TABLICY G£OSÓW
voice voices[NUM_OF_VOICES];
// INICJALIZACJA LFO
LFO lfo;

int main() {
	PmStream * midiStream;
	PmEvent MIDIbuffer[1];
	int note_number, velocity;
	srand(time(NULL));

	// USTAWIENIE PARAMETRÓW SYNTEZATORA
	// generatory tonalne
	tonalGenerators[0].chooseSignalType(SQUARE);
	tonalGenerators[1].chooseSignalType(SQUARE);
	tonalGenerators[2].chooseSignalType(SQUARE);
	tonalGenerators[0].setGain(.2f);
	tonalGenerators[1].setGain(.2f);
	tonalGenerators[2].setGain(.2f);
	tonalGenerators[0].setDetune(.0f);
	tonalGenerators[1].setDetune(-12.1f);
	tonalGenerators[2].setDetune(-12.0f);
	// generator szumu
	noiseGenerator.setGain(0);
	// ustawienie parametrów ADSR
	adsrs[TONAL_GENERATOR].setAttack(10);
	adsrs[TONAL_GENERATOR].setRelease(1000);
	adsrs[NOISE_GENERATOR].setAttack(5);
	adsrs[NOISE_GENERATOR].setRelease(10);
	adsrs[TONAL_GENERATORS_FILTER].setAttack(1000);
	adsrs[TONAL_GENERATORS_FILTER].setRelease(10);

	// LFO
	lfo.chooseSignalType(0);
	lfo.setFrequency(2.0f);
	lfo.setGain(1.0f);

	// parametry filtrów
	filterType tonalFilterType, noiseFilterType;
	float tonalPassbandFrequency, noisePassbandFrequency;
	tonalFilterType = lowPass;
	noiseFilterType = lowPass;
	tonalPassbandFrequency = 80.0f;
	noisePassbandFrequency = 400.0f;

	setTonalFilterParameters(tonalFilterType, tonalPassbandFrequency);
	setNoiseFilterParameters(noiseFilterType, noisePassbandFrequency);

	// przygotowanie urz¹dzenia MIDI
	midiStream = prepareMIDI();
	// przygotowanie urz¹dzenia audio
	PaStream *audioStream = prepareAudio();

	// G£ÓWNA PÊTLA 
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
			// sprawdŸ czy jest to nuta uderzona
			if (isNoteOn(MIDIbuffer[0].message)) {
				// w³¹cz g³os i przeka¿ mu numer dŸwiêku MIDI 
				setVoiceOn(note_number, velocity);
			}
			// jeœli nie jest uderzona to sprawdŸ czy jest puszczona
			else if (isNoteOff(MIDIbuffer[0].message)) {
				// wy³¹cz g³os, który gra t¹ nutê
				setVoiceOff(note_number);
			}
		}
	}
	// zamkniêcie urz¹dzenia MIDI
	Pm_Close(midiStream);

	// zamkniêcie urz¹dzenia Audio
	Pa_StopStream(audioStream);
	Pa_CloseStream(audioStream);
	Pa_Terminate();
	return 0;
}

// Funkcje przygotowuj¹ce obs³ugê MIDI i audio
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
	cout << "Wybierz urz¹dzenie MIDI: ";
	cin >> MIDI_device_number;

	return MIDI_device_number;
}

// Funkcje do obs³ugi zdarzeñ MIDI
bool isNoteOn(PmMessage msg) {	return (Pm_MessageStatus(msg) == NOTE_ON) && Pm_MessageData2(msg);}
bool isNoteOff(PmMessage msg) {	return (Pm_MessageStatus(msg) == NOTE_OFF) || !Pm_MessageData2(msg);}

// Funkcje do obs³ugi wyboru g³osu
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
	// funkcja zwraca numer pierwszego wolnego g³osu. Jeœli wszystkie s¹ zajête, zwraca numer pierwszego g³osu.
	int voice_number = 0;
	while (voices[voice_number].isPlayed != 0) {
		voice_number++;
	}
	// jeœli wszystkie zajête, kradniemy pierwszy g³os
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

// Funkcja wype³niaj¹ca bufor wyjœciowy karty dŸwiêkowej
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
		// Zsumuj g³osy i przepisz je do pierwszego kana³u bufora wyjœciowego
		for (voice_number = 0; voice_number < NUM_OF_VOICES; voice_number++)
			*out = *out + generateNewSample(voice_number);
		
		// Identyczn¹ wartoœæ próbki wpisz do drugiego kana³u
		*(++out) = *(out-1);
		// przesuñ wskaŸnik na kolejn¹ próbkê w buforze wyjœciowym
		out++;
	}
	return 0;
}

// Funkcja generuj¹ca nastêpn¹ próbkê sygna³u
float generateNewSample(int voice_number) {
	float tonal_sample = 0;
	float noise_sample = 0;
	float out_sample = 0;
	float delta;

	// 1. obliczenie próbki z generatorów tonalnych (sumowanie sygna³ów z kolejnych generatorów)
	for (int generator_number = 0; generator_number < 3; generator_number++) {
		tonal_sample += tonalGenerators[generator_number].wavetable[voices[voice_number].phases[generator_number]] * tonalGenerators[generator_number].gain;

		// obliczenie o ile przesun¹æ "fazê" - w oparciu o numer nuty MIDI danego g³osu oraz odstrojenie danego generatora
		delta = pow(2, float(voices[voice_number].note_number + tonalGenerators[generator_number].detune - 69.0f) / 12.0f) * 440.0f;
		// przesuniêcie tej "fazy"
		// UWAGA! 
		// a) chyba dobrym pomys³em bêdzie zmiana tej zmiennej "phase" na coœ co lepiej t³umaczy jej dzia³anie. Ale na razie nie mam pomys³u na jak¹ ;) 
		// b) wyrzuci³em na razie modulacjê FM. Ale to siê zrobi póŸniej :) 
		voices[voice_number].phases[generator_number] =
			(voices[voice_number].phases[generator_number] + int(delta + .5)) % TABLE_SIZE;
	}

	// 2. Obliczenie próbki pochodz¹cej z generatora szumu
	noise_sample = float(rand() % 100)/100 * noiseGenerator.gain;

	// 3. Obliczanie gain g³osu (w oparciu o ADSR) dla próbek pochodz¹cych z generatora tonalnego oraz szumowego
	voices[voice_number].updateGain(TONAL_GENERATOR);
	tonal_sample *= voices[voice_number].gain[TONAL_GENERATOR];
	voices[voice_number].tonalFilter.updateInSample(tonal_sample); // wstawienie aktualnej próbki do bufora próbek wejœciowych filtru tonalnego

	voices[voice_number].updateGain(NOISE_GENERATOR);
	noise_sample *= voices[voice_number].gain[NOISE_GENERATOR];
	voices[voice_number].noiseFilter.updateInSample(noise_sample);

	// 4. Filtracja 
	// uaktualnienie wspó³czynników filtra
	// voices[voice_number].updateGain(TONAL_GENERATORS_FILTER);
	// voices[voice_number].tonalFilter.setPassBandFrequency(voices[voice_number].gain[TONAL_GENERATORS_FILTER]*2000);
	
	// filtracja w³aœciwa
	tonal_sample = voices[voice_number].tonalFilter.Filtrate();
	noise_sample = voices[voice_number].noiseFilter.Filtrate();
	// wpisanie aktualnej próbki do bufora poprzednich próbek wyjœciowych filtru
	voices[voice_number].tonalFilter.updateOutSample(tonal_sample);
	voices[voice_number].noiseFilter.updateOutSample(noise_sample);

	// 5. LFO na amplitudzie
	out_sample = tonal_sample + noise_sample;
	out_sample *= lfo.wavetable[lfo.getPhase()];
	lfo.updatePhase();

	// 6. Sumowanie próbek sygna³u tonalnego i szumowego
	return out_sample;
}

bool textInterface() {
	char letter, option;
	int generator_number, signal_type, destination;
	float gain, detune, value, frequency;
	filterType filterType;
	cout << "Choose option:\n"
		<< "   q - quit\n"
		<< "   t - change tonal generators' options\n"
		<< "   n - change noise generator's options\n"
		<< "   a - change ADSR generator's options\n"
		<< "   l - change amplitude LFO's options\n"
		<< "   f - change filter parameters\n";
	cin >> letter;
	switch (letter) {
	case 'q':
		return 1;
		break;
	case 't':
		cout << "   Choose generator's number: ";
		cin >> generator_number;
		cout << "\n   Choose option:\n"
			<< "      s - signal type\n"
			<< "      g - gain\n"
			<< "      d - detune\n";
		cin >> option;
		switch (option) {
		case 's':
			cout << "\n      Choose signal type:\n"
				<< "         0 - sinus\n"
				<< "         1 - sawtooth\n"
				<< "         2 - square\n";
			cin >> signal_type;
			tonalGenerators[generator_number].chooseSignalType(signal_type);
			break;
		case 'g':
			cout << "\n      Enter gain: ";
			cin >> gain;
			tonalGenerators[generator_number].setGain(gain);
			break;
		case 'd':
			cout << "\n      Enter detune: ";
			cin >> detune;
			tonalGenerators[generator_number].setDetune(detune);
			break;
		default:
			break;
		}
		break;
	case 'n':
		cout << "\n   Enter gain: ";
		cin >> gain;
		noiseGenerator.setGain(gain);
		break;
	case 'a':
		cout << "\n   Choose destination:\n"
			<< "      0 - tonal generators\n"
			<< "      1 - noise generator";
		cin >> destination;
		cout << "\n   Choose option:\n"
			<< "      a - attack\n"
			<< "      r - release\n";
		cin >> option;
		cout << "Enter value: ";
		cin >> value;
		switch (option) {
		case 'a':
			adsrs[destination].setAttack(value);
			break;
		case 'r':
			adsrs[destination].setRelease(value);
			break;
		default:
			break;
		}
		break;
	case 'l':
		cout << "   Choose LFO option:\n "
			<< "      s - signal type\n"
			<< "      g - gain\n"
			<< "      f - frequency\n";
		cin >> option;
		switch (option) {
		case 's':
			cout << "\n      Choose signal type:\n"
				<< "         0 - sinus\n"
				<< "         1 - sawtooth\n"
				<< "         2 - square\n";
			cin >> signal_type;
			lfo.chooseSignalType(signal_type);
			break;
		case 'g':
			cout << "\n      Enter gain: ";
			cin >> gain;
			lfo.setGain(gain);
			break;
		case 'f':
			cout << "\n      Enter frequency: ";
			cin >> value;
			lfo.setFrequency(value);
			break;
		default:
			break;
		}
	case 'f':
		cout << "   Choose filter:\n "
			<< "      t - tonal generators' filter\n"
			<< "      n - noise generator's filter\n";
		cin >> option;
		switch (option)
		{
		case 't':
			filterType = lowPass;
			cout << "      Enter frequency: ";
			cin >> frequency;
			setTonalFilterParameters(filterType, frequency);
			break;
		case 'n':
			filterType = lowPass;
			cout << "      Enter frequency: ";
			cin >> frequency;
			setNoiseFilterParameters(filterType, frequency);
			break;
		default:
			break;
		}
	default:
		break;
	}
	return 0;
}

// funkcje ustawiaj¹ce parametry filtrów: tonalnego i szumowego
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

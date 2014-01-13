#include "PortMIDITest.h"

int main() {
	synthData xMasSynth;
	PmStream * midiStream;
	PmEvent MIDIbuffer[1];
	int note_number, velocity;

	// ustawienie parametrów syntezatora
	xMasSynth.setOscMixGain(.4, .1, .2);
	xMasSynth.setADRSParams(100, 1000);
	xMasSynth.setFMModulationParams(0, 4);

	// przygotowanie urz¹dzenia MIDI
	midiStream = prepareMIDI();

	// przygotowanie urz¹dzenia audio
	PaStream *audioStream = prepareAudio(xMasSynth);

	// g³ówna pêtla 
	while (1) {
		if (_kbhit())
			// jeœli zosta³ wciœniêty jakikolwiek klawisz na klawiaturze komputerowej - przerwij zabawê
			break;
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
				xMasSynth.setVoiceOn(note_number, velocity);
			}
			// jeœli nie jest uderzona to sprawdŸ czy jest puszczona
			else if (isNoteOff(MIDIbuffer[0].message)) {
				// wy³¹cz g³os, który gra t¹ nutê
				xMasSynth.setVoiceOff(note_number);
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
static int generateSignal(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData)
{
	// funkcja generuje sygna³ i przekazuje go do bufora wyjœciowego
	synthData* xMasSynth = (synthData*)userData;
	float *out = (float*)outputBuffer;
	unsigned int frame_number, voice_number;

	for (frame_number = 0; frame_number < framesPerBuffer; frame_number++)
	{
		*out = 0;
		// Zsumuj g³osy i przepisz je do pierwszego kana³u bufora wyjœciowego
		for (voice_number = 0; voice_number < NUM_OF_VOICES; voice_number++)
			*out = *out + xMasSynth->generateOutput(voice_number);
		// Identyczn¹ wartoœæ próbki wpisz do drugiego kana³u
		*(++out) = *(out-1);
		// przesuñ wskaŸnik na kolejn¹ próbkê w buforze wyjœciowym
		out++;
	}
	return 0;
}
bool isNoteOn(PmMessage msg) {	return (Pm_MessageStatus(msg) == NOTE_ON) && Pm_MessageData2(msg);}
bool isNoteOff(PmMessage msg) {	return (Pm_MessageStatus(msg) == NOTE_OFF) || !Pm_MessageData2(msg);}
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
PaStream* prepareAudio(synthData &synth) {
	PaStream *audioStream;
	Pa_Initialize();
	Pa_OpenDefaultStream(&audioStream,
		0,
		2,
		paFloat32,
		44100,
		128,
		generateSignal,
		&synth);
	Pa_StartStream(audioStream);

	return audioStream;
}

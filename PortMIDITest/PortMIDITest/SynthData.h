#include "WavetableGenerators.h"
#define NUM_OF_VOICES 3
#define CIRCLE_BUFFER_SIZE 2
#define FILTER_SIZE 2
struct voiceParameters
{
	int note_number = 0;
	float frequency = 0.0;
	float gain = 0.0;
	bool isPlayed = 0;
	int phase = 0;
};
struct adsrParameters
{
	// parametry odpowiadaj¹ce za brzmienie
	float attack = 100; // czas ataku [ms]
	float release = 1000; // czas release [ms]
};
struct oscMixGains
{
	float sin = 0;
	float saw = 0;
	float sqr = 0;
};
class synthData
{
private:
	WavetableGenerators waveTable;
	oscMixGains mix;
	adsrParameters adsr;
	float FMModulationDepth;
	float FMModulationIndex;
	float circleBuffer[NUM_OF_VOICES][CIRCLE_BUFFER_SIZE];
	int circleBufferIterator = 0;

public:
	float signal[NUM_OF_VOICES];
	voiceParameters voice[NUM_OF_VOICES];
	void updateGain(int voice_number);
	float generateOutput(int voice_number);
	void setVoiceOn(int note_number, int velocity);
	void setVoiceOff(int note_number);
	int chooseVoice();
	int findVoiceBynote_number(int note_number);

	void setOscMixGain(float sinMix, float sawMix, float sqrMix);
	void setADRSParams(float attack, float release);
	void setFMModulationParams(float depth, float index);
};

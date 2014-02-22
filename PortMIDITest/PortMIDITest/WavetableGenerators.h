#include "math.h"
#define SINUS 0
#define SAWTOOTH 1
#define SQUARE 2
#define NUMBER_OF_ADSR_DESTINATIONS 2

#define TABLE_SIZE 44100
#ifndef M_PI
	#define M_PI  (3.14159265)
#endif
enum filterType
{
	noFilter,
	lowPass,
	highPass
	// jakieœ jeszcze?
};

// TABLICE Z SYGNA£AMI
class wavetable
{
public:
	float sinus[TABLE_SIZE];
	float sawtooth[TABLE_SIZE];
	float square[TABLE_SIZE];
	
	wavetable();
	float* getTable(int signalType);
};
wavetable waveTables;

// PARAMETRY ADSR
class adsrParameters
{
public:
	// parametry odpowiadaj¹ce za brzmienie
	float attack; // = 100; // czas ataku [ms]
	float release; // = 1000; // czas release [ms]
	void setAttack(float a);
	void setRelease(float r);
};

// Tablica obiektów ADSR
adsrParameters adsrs[NUMBER_OF_ADSR_DESTINATIONS];

// PARAMETRY GENERATORÓW SYGNA£U
class generatorParameters
{
public:
	float gain = .1;
	void setGain(float g) { gain = g; }
};
// PARAMETRY GENERATORÓW SYGNA£U TONALNEGO (DZIEDZICZY z generatorParameters)
class tonalGeneratorParameters : public generatorParameters
{
public:
	float detune = 0;
	float wavetable[TABLE_SIZE];
	void setDetune(float d) { detune = d; }
	void chooseSignalType(int t);
};
// PARAMETRY GENERATORÓW SYGNA£U SZUMOWEGO (DZIEDZICZY z generatorParameters)
class noiseGeneretorParameters : public generatorParameters {};

// FILTRY
class Filter
{
public:
	void setFilterType( filterType type )
	{
		_type = type;
	}

	void updateInSample( float sample )
	{
		_prevInSamples[0] = _prevInSamples[1];
		_prevInSamples[1] = sample;
	}

	void updateOutSample( float sample )
	{
		_prevOutSamples[0] = _prevOutSamples[1];
		_prevOutSamples[1] = sample;
	}

	void setABParameters( float a, float b = 0.0f )
	{
		_a = a;
		_b = b;
	}

	void setPassBandFrequency(float frequency)
	{
		frequency = 2.0f * M_PI * frequency / 44100.0f;
		_a = (1 - sinf(frequency)) / cosf(frequency);
	}

	float Filtrate()
	{
		if (_type == noFilter)
			return _prevInSamples[1];
		if (_type == lowPass)
		{
			return (0.5f * (1 - _a) *
				(_prevInSamples[0] + _prevInSamples[1]) +
				(_a * _prevOutSamples[1]));
		}
	}

	//konstruktor
	Filter()
	{
		_type = noFilter;
		_a = 0.0f;
		_b = 0.0f;
		_prevInSamples[2] = { 0.0f };
		_prevOutSamples[2] = { 0.0f };
	}

private:
	filterType _type;
	float _a, _b; // parametry filtru
	float _prevInSamples[2];  // poprzednie próbki wejœciowe filtra 
	float _prevOutSamples[2]; // poprzednie próbki wyjœciowe filtra 
};
// G£OS
class voice
{
public:
	bool isPlayed = 0;
	int note_number = 0;
	float frequency = 0.0;
	float velocity;
	Filter tonalFilter;
	Filter noiseFilter;
	float gain[2];	// gain sygna³u tonalnego, szumu
	int phases[3];	// fazy dla kolejnych generatorów
	void updateGain(short destination);

	voice() {
		gain[2] = { 0 };
		phases[3] = { 0 };
	}
};


// ______________DEFINICJE________________________

float* wavetable::getTable(int signalType) {
	switch (signalType)
	{
	case SINUS:
		return sinus;
		break;
	case SAWTOOTH:
		return sawtooth;
		break;
	case SQUARE:
		return square;
		break;
	default:
		return sinus;
		break;
	}
}
wavetable::wavetable()
{
	for (int i = 0; i < TABLE_SIZE; i++)
	{
		sinus[i] = sin(2 * M_PI*float(i) / float(TABLE_SIZE));
		sawtooth[i] = 2 * float(i) / float(TABLE_SIZE) - 1;
		square[i] = (float)(i < TABLE_SIZE / 2) - (i >= TABLE_SIZE / 2);
	}
}

void adsrParameters::setAttack(float a) { attack = a; }
void adsrParameters::setRelease(float r) { release = r; }

void tonalGeneratorParameters::chooseSignalType(int t) {
	for (int i = 0; i<TABLE_SIZE; i++)
		wavetable[i] = *(waveTables.getTable(t) + i);
}

void voice::updateGain(short destination) {
	// attack
	if (isPlayed && (gain[destination] < 1))
		gain[destination] += 1 / (adsrs[destination].attack*44.100f);
	// release
	if (!isPlayed && (gain[destination] > 0))
		gain[destination] -= 1 / (adsrs[destination].release*44.100f);
}
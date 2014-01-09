#define NUM_OF_VOICES 3
class synthData
{
private:
	struct voiceParameters
	{
		int noteNumber = 0;
		float frequency = 0.0;
		float gain = 0.0;
		bool isPlayed = 0;
		int phase = 0;
	};
	struct adsrParameters //P: typ trzymaj¹cy parametry syntezatora
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

public:
	oscMixGains mix;
	adsrParameters adsr;
	float signal[NUM_OF_VOICES];
	voiceParameters voice[NUM_OF_VOICES];
	//float* sinus[];
};

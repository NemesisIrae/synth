#include "math.h"

#define TABLE_SIZE 44100
#ifndef M_PI
	#define M_PI  (3.14159265)
#endif
//class WavetableGenerators
//{
//public:
//
//	float sawtooth[TABLE_SIZE];
//	float square[TABLE_SIZE];

	void GenerateSineTable( float sinus[] )
	{
		for (int i = 0; i < TABLE_SIZE; i++)
		{
			sinus[i] = sin(2 * M_PI*double(i) / double(TABLE_SIZE));
		}
	}
	void GenerateSawtoothTable( float sawtooth[] )
	{
		for (int i = 0; i < TABLE_SIZE; i++)
		{
			sawtooth[i] = 2 * float(i) / float(TABLE_SIZE) - 1;
		}
	}
	void GenerateSquareTable( float square[] )
	{
		for (int i = 0; i < TABLE_SIZE; i++)
		{
			square[i] = (i < TABLE_SIZE / 2) - (i >= TABLE_SIZE / 2);
		}
	}

//};


	//sawtooth[i] = 2 * float(i) / float(TABLE_SIZE) - 1;
	//square[i] = (sinus[i] > 0) - (sinus[i] < 0);


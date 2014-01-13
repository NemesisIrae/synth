#include "math.h"

#define TABLE_SIZE 44100
#ifndef M_PI
	#define M_PI  (3.14159265)
#endif
class WavetableGenerators
{
public:

	float sinus[TABLE_SIZE];
	float sawtooth[TABLE_SIZE];
	float square[TABLE_SIZE];

	WavetableGenerators()
	{
		for (int i = 0; i < TABLE_SIZE; i++)
		{
			sinus[i] = sin(2 * M_PI*float(i) / float(TABLE_SIZE));
			sawtooth[i] = 2 * float(i) / float(TABLE_SIZE) -1;
			square[i] = (float)(i < TABLE_SIZE / 2) - (i >= TABLE_SIZE / 2);
		}
	}
};
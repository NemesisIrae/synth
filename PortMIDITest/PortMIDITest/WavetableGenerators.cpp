//#include "WavetableGenerators.h"
//
//void GenerateSineTable(float* sinus[])
//{
//	float sinus[TABLE_SIZE];
//	for (int i = 0; i < TABLE_SIZE; i++)
//	{
//		*sinus[i] = sin(2 * M_PI*double(i) / double(TABLE_SIZE));
//	}
//}
//
//void GenerateSawtoothTable(float* sawtooth[])
//{
//	//int sinus[TABLE_SIZE];
//	for (int i = 0; i < TABLE_SIZE; i++)
//	{
//		*sawtooth[i] = 2 * float(i) / float(TABLE_SIZE) - 1;
//	}
//}
//
//void GenerateSquareTable(float* square[])
//{
//	//int sinus[TABLE_SIZE];
//	for (int i = 0; i < TABLE_SIZE; i++)
//	{
//		*square[i] = (i < TABLE_SIZE / 2) - (i >= TABLE_SIZE / 2);
//	}
//}
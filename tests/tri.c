#include <stdio.h>
#include <stdlib.h>
#include <math.h>

float pwmfreq = 78125;
float slice = 1 / pwmfreq;
float triFreq = 1000;
float triPeriod = pwmfreq / triFreq;
float triAmp = 1;
float accum = 0;

int halfAmp = 512;
int fullAmp = 1024;

//square(t) = sgn(sin(2πt))
//sawtooth(t) = t - floor(t + 1/2)
//triangle(t) = abs(sawtooth(t))

float sawtooth(float t)
{
	return t - floor(t + (t/2));
}

int triAsInt()
{
	accum += 1; // assumption - each call is +1 freq
	if (accum > triFreq)
	{
		accum -= triFreq;
	}
	//float x = triAmp * ((accum*2 - triPeriod) / triPeriod);

	float xNrm = accum / triPeriod;
	float y = -triAmp + (abs(0.5 - xNrm) * 2 * triAmp);
	return halfAmp + (halfAmp * x);
}

int main()
{

	for (int i = 0; i < (int) triPeriod; ++i)
	{
	}
	return 0;
}

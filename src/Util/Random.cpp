#include "Util/Random.hpp"

#include <ctime>
#include <random>

void Random::Seed()
{
	srand((unsigned)time(NULL));
}

float Random::Float(float max)
{
	return (float)rand() / (float)(RAND_MAX / max);
}

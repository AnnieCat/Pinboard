#include "my.h"

void showABunchOfSums()
{
	for (int i = 3; i < 10; i += 2)
	{
		for (int j = 6; j < 9; ++j)
		{
			showMeTheSumOf(i, j);
		}
	}
}

void SumResult::Compute(int newA, int newB)
{
	a = newA;
	b = newB;
	sum = a + b*b;
}

void SumResult::Print()
{
	printf("The sum of %d and %d is %d, dawg!\n", a, b, sum);
}

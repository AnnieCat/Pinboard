#include "my.h"

void showMeTheSumOf(int a, int b)
{
	int sum = a + b;
	printf("The sum of %d and %d is %d\n", a, b, sum);
}

void PrintThreeTimes(SumResult r)
{
	r.Print();
	r.Print();
	r.Print();
}
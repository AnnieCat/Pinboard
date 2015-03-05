#include <cstdio>

class SumResult
{
	int a, b, sum;
public:
	void Compute(int newA, int newB);
	void Print();
};

void PrintThreeTimes(SumResult r);
void showMeTheSumOf(int a, int b);
void showABunchOfSums();
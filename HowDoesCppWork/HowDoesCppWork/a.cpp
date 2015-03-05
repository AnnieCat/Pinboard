#include "my.h"

#include "../HowDoDllsWork/HowDoDllsWork.h"

int main()
{
	SumResult r;
	r.Compute(9, 99);
	PrintThreeTimes(r);

	showMeTheSumOf(3, 5);
	showABunchOfSums();
	printf("Hello there!\n");

	MySpecialFunctionThatLivesInADll();

	return 0;
}
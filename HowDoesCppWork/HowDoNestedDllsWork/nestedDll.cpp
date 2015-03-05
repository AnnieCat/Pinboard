#include <cstdio>

__declspec(dllexport) void ANestedDllFunction()
{
	printf("I'm a function inside a DLL which will be CALLED from ANOTHER DLL.\n");
}